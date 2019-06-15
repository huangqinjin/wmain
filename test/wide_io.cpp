#include <iostream>
#include <string>
#include <stdio.h>   // _fileno
#include <io.h>      // _get_osfhandle, _setmode
#include <fcntl.h>   // _O_U16TEXT, _O_U8TEXT
#include <windows.h>

using namespace std;

void utf16()
{
    /**
     * Using UTF-16 IO, the sink and source are UTF-16 encoded.
     *   - File I/O: ReadFile/WriteFile
     *   - Console Input: ReadConsoleW
     *   - Console Output: WriteConsoleW for each wchar, so only supports UCS-2
     */
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    // write BOM (useful if output is redirected to file)
    wchar_t bom = '\xFF\xFE';
    wcout.write(&bom, 1);
    wcerr.write(&bom, 1);

    wstring s = L"UTF-16 [鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
    do {
       wcout << s << endl;
       wcerr << s << endl;
    } while (getline(wcin, s));
}

void utf8()
{
    /**
     * Using UTF-8 IO, the sink and source are UTF-8 encoded.
     *   - Input: ReadFile -> UTF-8 -> UTF-16
     *   - File Output: UTF-16 -> UTF-8 -> WriteFile
     *   - Console Output: same as UTF-16. WriteConsoleW for each wchar, so only supports UCS-2
     * 
     * Note: ReadFile/ReadConsoleA get ANSI characters from ConsoleInputCP, but SetConsoleCP(CP_UTF8) 
     * doesn't work since it only supports DBCS. See also
     * https://github.com/microsoft/terminal/blob/master/src/host/dbcs.cpp#TranslateUnicodeToOem.
     * The perfect solution is that using UTF-8 mode if stdin is redirected, or UTF-16 for console.
     * It should be fixed in UCRT to use ReadConsoleW as same as UTF-16 mode rather than ReadFile.
     */
    int fdInput = _fileno(stdin);
    HANDLE hInput = reinterpret_cast<HANDLE>(_get_osfhandle(fdInput));
    DWORD mode;
    mode = GetConsoleMode(hInput, &mode) ? _O_U16TEXT : _O_U8TEXT;
    _setmode(fdInput, mode);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    wstring s = L"UTF-8 [鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
    do {
        wcout << s << endl;
        wcerr << s << endl;
    } while (getline(wcin, s));
}

int wmain(int argc, wchar_t* argv[])
{
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/setmode#remarks
    // https://stackoverflow.com/questions/8947949/mixing-cout-and-wcout-in-same-program
    // Unicode translation mode of file is for wide IO functions and narrow is not supported.
    // Wide IO functions use direct I/O and have nothing to do with ACP and OEMCP.

    switch (argv[1] ? argv[1][0] : L'0')
    {
        case L'1': utf16(); break;
        case L'2': utf8(); break;
        default: wcout << L"1: UTF-16\n2: UTF-8\n";
    }
}
