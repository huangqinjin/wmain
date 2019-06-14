#include <windows.h>

static DWORD WINAPI cRead(HANDLE console, LPWSTR buf, DWORD size)
{
    // Only work for unicode version of ReadConsole?
    CONSOLE_READCONSOLE_CONTROL ctrl = {   
        .nLength = sizeof(CONSOLE_READCONSOLE_CONTROL),
        .nInitialChars = 0,
        // https://stackoverflow.com/questions/43836040/win-api-readconsole
        .dwCtrlWakeupMask = (1 << ('z' - 'a' + 1)) | (1 << ('d' - 'a' + 1)),  // Ctrl-Z or Ctrl-D
        .dwControlKeyState = 0,  // set by ReadConsole
    };
    if (!ReadConsoleW(console, buf, size, &size, &ctrl) || size == 0 ||
        buf[size - 1] == ('z' - 'a' + 1) || buf[size - 1] == ('d' - 'a' + 1))
        size = 0;
    return size;
}

static void WINAPI cWrite(HANDLE console, LPCWSTR buf, DWORD size)
{
    WriteConsoleW(console, buf, size, NULL, NULL);
}

static DWORD WINAPI fRead(HANDLE file, LPWSTR buf, DWORD size)
{
    if (ReadFile(file, buf, size * sizeof(buf[0]), &size, NULL))
        size = (size + sizeof(buf[0]) - 1) / sizeof(buf[0]);
    else
        size = 0;
    return size;
}

static void WINAPI fWrite(HANDLE file, LPCWSTR buf, DWORD size)
{
    WriteFile(file, buf, size * sizeof(buf[0]), NULL, NULL);
}

int wmain()
{
    typedef DWORD (WINAPI *ReadF)(HANDLE console, LPWSTR buf, DWORD size);
    typedef void (WINAPI *WriteF)(HANDLE console, LPCWSTR buf, DWORD size);

    DWORD c;

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hError = GetStdHandle(STD_ERROR_HANDLE);

    ReadF in = GetConsoleMode(hInput, &c) ? cRead : fRead;
    WriteF out = GetConsoleMode(hOutput, &c) ? cWrite : fWrite;
    WriteF err = GetConsoleMode(hError, &c) ? cWrite : fWrite;

    WCHAR buf[] = L"[鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
    c = ARRAYSIZE(buf);
    buf[c - 1] = L'\n';
    do {
        out(hOutput, buf, c);
        out(hError, buf, c);
    } while (c = in(hInput, buf, ARRAYSIZE(buf)));

    return 0;
}
