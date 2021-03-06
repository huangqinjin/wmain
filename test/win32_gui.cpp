#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <Richedit.h> // MSFTEDIT_CLASS
#include <strsafe.h>  // StringCchPrintfEx, StringCchCopy
#include <process.h>  // _beginthreadex, _endthreadex
#include <iostream>
#include <string>

// https://docs.microsoft.com/en-us/windows/win32/controls/cookbook-overview
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

WCHAR szWindowTitle[MAX_PATH + 1000];
WCHAR* pszAdditionalTitle;
std::size_t nMaxAdditionalTitle;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define IDC_CONSOLE_BUTTON          1
#define IDC_CONSOLE_UNICODE_EDIT    2
#define IDC_CONSOLE_ANSI_EDIT       3

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    const WCHAR szWindowClassW[] = L"Sample Window Class";
    const CHAR szWindowClassA[] = "Sample Window Class";

    // Register the window class:
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = (HCURSOR)LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    switch(lpCmdLine[0])
    {
    case 'Y':
    case 'y':
    case '1':
    unicode:
        wc.lpszClassName = szWindowClassW;
        RegisterClassExW(&wc);
        break;

    case 'N':
    case 'n':
    case '2':
    ansi:
        static_assert(sizeof(WNDCLASSEXW) == sizeof(WNDCLASSEXA), "WNDCLASSEX size error");
        wc.lpszClassName = (LPCWSTR)szWindowClassA;
        RegisterClassExA((WNDCLASSEXA*)&wc);
        break;

    default:
        int r = MessageBoxA(NULL,
            "Valid Command Line:\r"
            "Y/y/1: Create Unicode Window\r"
            "N/n/2: Create ANSI Window",
            lpCmdLine, MB_YESNOCANCEL | MB_ICONINFORMATION);
        if (r == IDYES) goto unicode;
        if (r == IDNO) goto ansi;
        return 0;
    }

    // Current ANSI code page name as winsow title:
    CPINFOEXW cp;
    GetCPInfoExW(CP_ACP, 0, &cp);
    StringCchPrintfExW(szWindowTitle, ARRAYSIZE(szWindowTitle), &pszAdditionalTitle,
                       &nMaxAdditionalTitle, 0, L"ANSI Code Page: %ls  ", cp.CodePageName);

    // Create and display the main window:
    HWND hWnd = CreateWindowExW(
        0,                              // Optional window styles
        szWindowClassW,                 // Window class
        szWindowTitle,                  // Window title
        WS_OVERLAPPEDWINDOW,            // Window style
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

class Console
{
    CRITICAL_SECTION loglock;
    const char* const logfile = "console.log";
    HWND hWnd = NULL;
    HWND hConsole = NULL;
    UINT newcp = 0;
    UINT oldcp = 0;
    HANDLE hThread = NULL;

    static unsigned __stdcall input(void* arg)
    {
        Console* c = (Console*)arg;
        c->log("begin");

        std::wstring ws = L"[鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
        std::string s;
        do {
            s.resize(WideCharToMultiByte(c->newcp, 0, ws.data(), (int)ws.size(), NULL, 0, NULL, NULL));
            WideCharToMultiByte(c->newcp, 0, ws.data(), (int)ws.size(), s.data(), (int)s.size(), NULL, NULL);
            std::cout << s << std::endl;

            for (int i = 0; i < 2; ++i)
            {
                bool unicode = (i == 0);
                WCHAR ch = 1;
                HWND hEdit = GetDlgItem(c->hWnd, unicode ? IDC_CONSOLE_UNICODE_EDIT : IDC_CONSOLE_ANSI_EDIT);
                LRESULT r = SendMessageW(hEdit, EM_GETLINECOUNT, 0, 0);
                SendMessageW(hEdit, EM_GETLINE, r-1, (LPARAM)&ch);

                auto Append = [hEdit, unicode](const void* str) {
                    const WCHAR eol[2] = { L'\r', 0 };
                    SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                    (unicode ? SendMessageW : SendMessageA)(hEdit, EM_REPLACESEL, FALSE, (LPARAM)str);
                    SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)&eol);
                };

                // last line is not empty
                if (ch != L'\r') Append(nullptr);
                Append(unicode ? (void*)ws.data() : (void*)s.data());

                // Scoll to bottom
                while (LOWORD(SendMessageW(hEdit, EM_SCROLL, SB_PAGEDOWN, 0)));
            }

            StringCchCopyW(pszAdditionalTitle, nMaxAdditionalTitle, ws.data());
            SetWindowTextW(c->hWnd, szWindowTitle);

        } while (getline(std::wcin, ws));

        c->log("end");
        _endthreadex(0);
        return 0;
    }

    static BOOL WINAPI handler(DWORD dwCtrlType)
    {
        Console* c = Console::instance();
        c->log("ctrl %u", dwCtrlType);
        if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT)
        {
            // Note that handler is called in a new thread and SendMessage will result in dead lock.
            // seems that this thread is synchronized with FreeConsole().
            PostMessage(instance()->hWnd, WM_COMMAND, MAKEWPARAM(IDC_CONSOLE_BUTTON, BN_CLICKED), 0);
            // TRUE to prevent default handler terminates the process since the posted message above
            // is likely not handled yet.
            return TRUE;
        }
        // Whether TRUE or FALSE, CTRL_CLOSE_EVENT, CTRL_LOGOFF_EVENT, and CTRL_SHUTDOWN_EVENT
        // always terminate the process.
        return FALSE;
    }

    Console()
    {
        InitializeCriticalSection(&loglock);
        remove(logfile);
    }

    ~Console()
    {
        DeleteCriticalSection(&loglock);
    }

public:
    static Console* instance()
    {
        static Console c;
        return &c;
    }

    void log(const char* fmt, ...)
    {
        SYSTEMTIME st;
        FILETIME ft, lt;
        GetSystemTimePreciseAsFileTime(&ft);
        FileTimeToLocalFileTime(&ft, &lt);
        FileTimeToSystemTime(&lt, &st);

        ULARGE_INTEGER v;
        v.HighPart = lt.dwHighDateTime;
        v.LowPart = lt.dwLowDateTime;
        DWORD wFraction = v.QuadPart % 10000; // representing the number of 100-nanosecond intervals

        EnterCriticalSection(&loglock);
        FILE* f;
        fopen_s(&f, logfile, "a");
        fprintf(f, "%02d:%02d:%02d.%03d%04d %d - ", st.wHour, st.wMinute, st.wSecond,
                st.wMilliseconds, wFraction, GetCurrentThreadId());
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fputc('\n', f);
        fclose(f);
        LeaveCriticalSection(&loglock);
    }

    BOOL open(HWND hWnd)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole())
            return FALSE;
        log("open");
        this->hWnd = hWnd;
        hConsole = GetConsoleWindow();
        // SetWindowPos(hConsole, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        // Disable Close button since there is no way to detach before the console window closed.
        // https://devblogs.microsoft.com/oldnewthing/20100604-00/?p=13803
        EnableMenuItem(GetSystemMenu(hConsole, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        SetConsoleCtrlHandler(&Console::handler, TRUE);

        DWORD dwMode;
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hInput, &dwMode);
        SetConsoleMode(hInput, dwMode & ~ENABLE_QUICK_EDIT_MODE);

        newcp = GetACP();
        oldcp = GetConsoleOutputCP();
        SetConsoleOutputCP(newcp);

        FILE* useless;
        freopen_s(&useless, "CONOUT$", "w+t", stdout);
        freopen_s(&useless, "CONOUT$", "w+t", stderr);
        freopen_s(&useless, "CONIN$", "r+t,ccs=UTF-16LE", stdin);

        // Clear the error state
        std::ios_base* ios[] = {
            &std::cin, &std::wcin, &std::cout, &std::wcout,
            &std::cerr, &std::wcerr, &std::clog, &std::wclog,
        };
        for (auto io : ios) io->clear();

        printf("***************************************************************************\n"
               "Disable `QuickEdit Mode`: https://bugs.python.org/issue26744. \n"
               "Under this mode, left clicking on console window is to select text for copy\n"
               "and console window title changed to 'Select ...'. So output will be blocked\n"
               "to keep content of window until right clicking to finish selection.\n"
               "***************************************************************************\n\n\n"
        );

        DWORD dwProcessId;
        GetWindowThreadProcessId(hConsole, &dwProcessId);
        if (GetCurrentProcessId() == dwProcessId)
        {
            printf("*****************************************************************\n"
                   "Windows Console cannot display some complex glyphs due to lack of\n"
                   "font-fallback support. Consider using Windows Terminal and run\n"
                   "       start /wait /path/to/exe\n"
                   "*****************************************************************\n\n\n"
            );
        }

        printf("Unicode: %s\nANSI CP: %d\nConsole CP: %d\nlocale: %s\n",
               IsWindowUnicode(hWnd) ? "TRUE" : "FALSE", newcp, oldcp,
               setlocale(LC_ALL, nullptr));

        hThread = (HANDLE)_beginthreadex(NULL, 0, &Console::input, this, 0, NULL);
        return TRUE;
    }

    BOOL is_open()
    {
        return hConsole != NULL;
    }

    void close()
    {
        log("close");
        // std::cin.clear(std::ios_base::eofbit);
        SetConsoleOutputCP(oldcp);
        GetSystemMenu(hConsole, TRUE);
        DWORD c = GetConsoleProcessList(&c, 1);
        // Detach console first to avoid current process being terminated
        // when the console window gets closed.
        FreeConsole();
        log("detach");
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
        log("wait");
        // Close the console window if no more processes attached to it.
        if (c <= 1) SendMessageW(hConsole, WM_CLOSE, 0, 0);
        hConsole = NULL;
    }
};


// Processe messages for the main window
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        CreateWindowExW(
            0,
            L"BUTTON",  // Predefined class
            Console::instance()->is_open() ? L"Close" : L"Open",   // Button text
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles
            10,         // x position
            10,         // y position
            100,        // Button width
            50,        // Button height
            hWnd,       // Parent window
            (HMENU)IDC_CONSOLE_BUTTON,       // Child window ID
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL        // Pointer not needed
        );

        LoadLibraryW(L"Msftedit.dll");
        CreateWindowExW(
            0,
            L"STATIC",  // Predefined class
            L"Unicode", // Text
            SS_SUNKEN | WS_VISIBLE | WS_CHILD,  // Styles
            10,         // x position
            80,         // y position
            60,         // Label width
            30,         // Label height
            hWnd,       // Parent window
            NULL,       // Child window ID
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL        // Pointer not needed
        );
        CreateWindowExW(
            0,
            MSFTEDIT_CLASS,  // Predefined class
            NULL,   // Text
            ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,  // Styles
            10,         // x position
            100,        // y position
            300,        // Edit width
            300,        // Edit height
            hWnd,       // Parent window
            (HMENU)IDC_CONSOLE_UNICODE_EDIT,       // Child window ID
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL        // Pointer not needed
        );

        CreateWindowExW(
            0,
            L"STATIC",  // Predefined class
            L"ANSI",    // Text
            SS_SUNKEN | WS_VISIBLE | WS_CHILD,  // Styles
            350,        // x position
            80,         // y position
            60,         // Label width
            30,         // Label height
            hWnd,       // Parent window
            NULL,       // Child window ID
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL        // Pointer not needed
        );
        CreateWindowExW(
            0,
            MSFTEDIT_CLASS,  // Predefined class
            NULL,   // Text
            ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,  // Styles
            350,        // x position
            100,        // y position
            300,        // Edit width
            300,        // Edit height
            hWnd,       // Parent window
            (HMENU)IDC_CONSOLE_ANSI_EDIT,       // Child window ID
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL        // Pointer not needed
        );
        break;

    case WM_COMMAND:
        switch (const int wmId = LOWORD(wParam))
        {
        case IDC_CONSOLE_BUTTON:
            if (Console::instance()->is_open())
            {
                Console::instance()->close();
                SetDlgItemTextW(hWnd, wmId, L"Open");
            }
            else if (Console::instance()->open(hWnd))
            {
                SetWindowTextW((HWND)lParam, L"Close");
            }
            else
            {
                MessageBoxW(hWnd, L"Attach and Alloc Console Failed", L"Open Console", MB_OK);
            }
            break;

        default:
            goto DEFAULT;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
        if (Console::instance()->is_open())
            Console::instance()->close();

    DEFAULT:
    default:
        return (IsWindowUnicode(hWnd) ? DefWindowProcW : DefWindowProcA)(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
