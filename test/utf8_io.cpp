#include <windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>   // quoted
#include <string>
#include <clocale>
#include <cassert>
#include <filesystem>
#include <stdio.h>   // _fileno
#include <io.h>      // _get_osfhandle, _setmode
#include <fcntl.h>   // _O_U16TEXT

// https://en.cppreference.com/w/cpp/language/class_template
// Explicit instantiation definitions ignore member access specifiers.
#define HIJACKER(Class, Member, Type)                     \
inline Type Class::* Class ## _ ## Member();              \
template<Type Class::* p> struct Class ## Member {        \
friend Type Class::* Class ## _ ## Member() { return p; } \
}; template struct Class ## Member<&Class::Member>;

namespace std
{
    HIJACKER( filebuf, _Myfile, FILE*)
    HIJACKER(wfilebuf, _Myfile, FILE*)
}

FILE* _get_cfile(const std::filebuf* buf)
{
    return buf ? buf->*std::filebuf__Myfile() : nullptr;
}

FILE* _get_cfile(const std::wfilebuf* buf)
{
    return buf ? buf->*std::wfilebuf__Myfile() : nullptr;
}

template<typename CharT, typename Traits>
FILE* _get_cfile(const std::basic_streambuf<CharT, Traits>* buf)
{
    return _get_cfile(dynamic_cast<const std::basic_filebuf<CharT, Traits>*>(buf));
}

using namespace std;
namespace fs = std::filesystem;

struct ConsoleInputCP
{
    const UINT oldcp;
    const UINT newcp;
    ~ConsoleInputCP() { if (oldcp != newcp) SetConsoleCP(oldcp); }
    explicit ConsoleInputCP(const UINT cp = CP_UTF8) : oldcp(GetConsoleCP()), newcp(cp ? cp : oldcp)
    { 
        if (oldcp != newcp) SetConsoleCP(newcp); 
    }
};

struct ConsoleOutputCP
{
    const UINT oldcp;
    const UINT newcp;
    ~ConsoleOutputCP() { if (oldcp != newcp) SetConsoleOutputCP(oldcp); }
    explicit ConsoleOutputCP(const UINT cp = CP_UTF8) : oldcp(GetConsoleOutputCP()), newcp(cp ? cp : oldcp)
    {
        if (oldcp != newcp) SetConsoleOutputCP(newcp);
    }
};

struct ConsoleCP : ConsoleInputCP, ConsoleOutputCP {};

bool narrow(string& s)
{
    static const UINT cp = [] {
        if (FILE* f = _get_cfile(cin.rdbuf()))
        {
            HANDLE hInput = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(f)));
            DWORD mode;
            if (GetConsoleMode(hInput, &mode))
                return GetConsoleCP();
        }
        return (UINT)0;
    }();

    if (getline(cin, s))
    {
        if (cp)
        {
            static wstring ws;
            ws.resize(MultiByteToWideChar(cp, 0, s.data(), s.size(), NULL, 0));
            MultiByteToWideChar(cp, 0, s.data(), s.size(), ws.data(), ws.size());
            s.resize(WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.size(), NULL, 0, NULL, NULL));
            WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.size(), s.data(), s.size(), NULL, NULL);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool wide(string& s)
{
    static const bool wide = [] {
        if (FILE* f = _get_cfile(cin.rdbuf()))
        {
            int fd = _fileno(f);
            HANDLE hInput = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
            DWORD mode;
            if (GetConsoleMode(hInput, &mode))
            {
                assert(f == _get_cfile(wcin.rdbuf()) && "delegating cin to wcin requires the same underlying FILE of them");
                _setmode(fd, _O_U16TEXT);
                return true;
            }
        }
        return false;        
    }();

    if (wide)
    {
        static wstring ws;
        if (getline(wcin, ws))
        {
            s.resize(WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.size(), NULL, 0, NULL, NULL));
            WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.size(), s.data(), s.size(), NULL, NULL);
            return true;
        }
        return false;
    }
    else
    {
        return (bool)getline(cin, s);
    }
}

void use_c_locale()
{
    // SetConsoleOutputCP(CP_UTF8) to display characters correctly due to the encoding conversion from
    // ConsoleOutputCP to Unicode in Windows Console. Narrow and wide input both work. Sine narrow input
    // only supports DBCS, some Unicode characters will be replaced by the question mark '?'.
    ConsoleOutputCP cp;
    cout << "previous console output codepage: " << cp.oldcp << endl;
    cout << "set console output codepage to UTF-8: " << CP_UTF8 << endl;

    // The encoding of std::filesystem::path::string() is ACP, explicit UTF-8 conversion is needed.
    cout << std::quoted(reinterpret_cast<const string&>(fs::current_path().u8string())) << endl;

    string s = "[鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
    do {
        cout << s << endl;
        cerr << s << endl;
    } while (narrow(s));
}

void use_utf8_locale()
{
    // SetConsoleOutputCP(CP_UTF8) to display characters correctly due to the encoding conversion from
    // ConsoleOutputCP to Unicode in Windows Console. SetConsoleCP(CP_UTF8) due to the bug of double
    // translation for console output before UCRT 10.0.19041.0. Use wide() for input since narrow input
    // only supports DBCS.
    ConsoleCP cp;
    cout << "previous console input codepage: " << cp.ConsoleInputCP::oldcp << endl;
    cout << "previous console output codepage: " << cp.ConsoleOutputCP::oldcp << endl;
    cout << "set console codepage to UTF-8: " << CP_UTF8 << endl;
    // Supports Windows XP. __acrt_get_qualified_locale() handles ".utf8" specially but 
    // __acrt_get_qualified_locale_downlevel() for Windows before Vista not.
    // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/setlocale-wsetlocale
    cout << "set locale to: " << setlocale(LC_CTYPE, ".65001") << endl;

    // The encoding of std::filesystem::path::string() here is CP_UTF8.
    cout << fs::current_path() << endl;

    string s = "[鑫][靐][龘][䲜][䨻][𠔻][𪚥]";
    do {
        cout << s << endl;
        cerr << s << endl;
    } while (wide(s));
}

int main(int argc, char* argv[])
{
    /**
     * Using ANSI IO, the sink and source are ANSI (including UTF-8) encoded.
     *   - Input: ReadFile
     *   - File Output: WriteFile
     *   - Console Output with LC_CTYPE:
     *      + C:  WriteFile
     *      + utf8:  UTF-8 -> UTF-16 -> ConsoleOutputCP -> WriteFile
     *      + otherwise: DBCS (mbtowc) -> UTF-16 -> ConsoleOutputCP -> WriteFile
     * 
     * In UCRT 10.0.17134.0, setting locale to utf8 is supported, see
     * https://github.com/huangqinjin/ucrt/blob/10.0.17134.0/locale/get_qualified_locale.cpp.
     * With utf8 locale, some POSIX functions use CP_UTF8 instead of ACP for doing narrow->wide
     * conversions. The codepage selection logic is implemented in
     * https://github.com/huangqinjin/ucrt/blob/10.0.17134.0/inc/corecrt_internal_win32_buffer.h#__acrt_get_utf8_acp_compatibility_codepage.
     * An example is fopen: it convert narrow path to wide path using the selected codepage and
     * then delegates to wide version of open, see
     * https://github.com/huangqinjin/ucrt/blob/10.0.17134.0/lowio/open.cpp#_sopen_nolock.
     * And, the encoding of std::filesystem::path::string() is the same logic.
     * 
     * Since UCRT 10.0.17763.0, print functions treat the text data as UTF-8 encoded if locale
     * is set to utf8, see
     * https://github.com/huangqinjin/ucrt/blob/10.0.17763.0/lowio/write.cpp#write_double_translated_ansi_nolock
     * 
     * Before UCRT 10.0.19041.0, the double translation for console output uses ConsoleInputCP by mistake. Double
     * translation is no need. Should be reworked to use WriteConsoleW after translated to UTF-16 such
     * that no codepage is involved: ANSI -> UTF-16 -> WriteConsoleW.
     * 
     * Same as Wide UTF-8 IO, ReadFile/ReadConsoleA get ANSI characters from ConsoleInputCP,
     * but SetConsoleCP(CP_UTF8) doesn't work since it only supports DBCS. See also
     * https://github.com/microsoft/terminal/blob/master/src/host/dbcs.cpp#TranslateUnicodeToOem.
     * There are two workaround: delegating to wide input or doing ConsoleInputCP -> UTF-16 -> UTF-8
     * conversion. UCRT should implement input as the reverse process of reworked output, i.e.
     * ReadConsoleW -> UTF-16 -> ANSI (including UTF-8).
     */

    switch (argc > 1 ? argv[1][0] : '0')
    {
        case '1': use_c_locale(); break;
        case '2': use_utf8_locale(); break;
        default: cout << "1: Use C locale\n2: Use UTF-8 locale\n";
    }
}
