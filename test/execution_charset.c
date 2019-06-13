//------------------ Encoding: Windows-1252 ------------------
#include <windows.h>
#include <string.h>
#include <tchar.h>   // for _t*, _T*, controlled by _UNICODE
#include <stdio.h>   // _fileno
#include <io.h>      // _setmode
#include <fcntl.h>   // _O_U16TEXT

#define MAX_MARKED_LENGTH 100
// https://en.wikipedia.org/wiki/Windows-1252#Character_set
const char ansi[] = "MARKER{[€][‰][§][ÿ]}MARKER";
// CP1252 codepoints for corresponding characters
const char cp1252[] = "[\x80][\x89][\xA7][\xFF]";
// CP936 codepoints for corresponding characters, substitute '?' for '\xFF' that cannot be represented
const char cp936[] = "[\x80][\xA1\xEB][\xA1\xEC][?]";

static long read_marked_content(const _TCHAR* path, const char* marker, char marked[MAX_MARKED_LENGTH]);

int _tmain(int argc, _TCHAR* argv[])
{
    (void) argc;
#ifdef _UNICODE
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif

    char data[MAX_MARKED_LENGTH];
    read_marked_content(_T(__FILE__), "MARKER", data);  // read from source file
    _tprintf(_T("source file is cp1252? %d\n"), strcmp(data, cp1252) == 0);
    read_marked_content(argv[0], "MARKER", data);       // read from executable file
    _tprintf(_T("executable file is cp936? %d\n"), strcmp(data, cp936) == 0);

    // test and verify `source-charset -> Unicode -> execution-charset` conversion done by Visual Studio
    BOOL UsedDefaultChar = FALSE;
    wchar_t unicode[MAX_MARKED_LENGTH];
    // source-charset(CP1252) -> Unicode
    MultiByteToWideChar(1252, 0, cp1252, sizeof(cp1252), unicode, ARRAYSIZE(unicode));
    // Unicode -> execution-charset(CP936)
    WideCharToMultiByte(936, 0, unicode, -1, data, sizeof(data), NULL, &UsedDefaultChar);
    if (UsedDefaultChar)
    {
        char DefaultChar[MAX_DEFAULTCHAR + 1] = {0};
        CPINFO info;
        GetCPInfo(936, &info);
        memcpy(DefaultChar, info.DefaultChar, MAX_DEFAULTCHAR);
        _tprintf(_T("Unicode characters that cannot be represented in CP936 are substituted by [%hs]\n"), DefaultChar);
    }
    _tprintf(_T("conversion is expected? %d\n"), strcmp(data, cp936)  == 0);

    return 0;
}

long read_marked_content(const _TCHAR* path, const char* marker, char marked[MAX_MARKED_LENGTH])
{
    // get file content
    FILE* file = _tfopen(path, _T("rb"));
    if (file == NULL)
    {
        _tprintf(_T("cannot open [%Ts]\n"), path);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* content = malloc(size + 1);
    fread(content, 1, size, file);
    fclose(file);

    char* content2 = malloc(size);
    memcpy(content2, content, size);

    // make content null-ternimated string
    for(long i = 0; i < size; ++i)
        if (content[i] == 0) content[i] = 1;
    content[size] = 0;
    size = 0;

    // make left marker
    size_t len = strlen(marker);
    strcpy(marked, marker);
    marked[len] = '{';
    marked[len + 1] = 0;

    // search left marker
    char* begin = strstr(content, marked);
    if (!begin) goto error;

    // make right marker
    strcpy(marked + 1, marker);
    marked[0] = '}';

   // search right marker
    begin += len + 1;
    char* end = strstr(begin, marked);
    if (!end) goto error;

    // copy out
    size = (long)(end - begin);
    if (size >= MAX_MARKED_LENGTH) goto error;
    memcpy(marked, content2 + (begin - content), size);
    goto exit;

error:
    if (size != 0)
    {
        _tprintf(_T("no marker in [%Ts]\n"), path);
        size = 0;
    }
    else
    {
        _tprintf(_T("too large between markers in [%Ts]\n"), path);
    }

exit:
    free(content);
    free(content2);
    marked[size] = 0;
    return size;
}
