#include <windows.h>
#include <stdio.h>
#include <string.h>

const wchar_t words[] = L"[鑫][靐][龘][䲜][䨻][𠔻][𪚥]";

int main()
{
    // https://docs.microsoft.com/en-us/cpp/cpp/char-wchar-t-char16-t-char32-t
    //> The wchar_t type is an implementation-defined wide character type.
    //> In the Microsoft compiler, it represents a 16-bit wide character used
    //> to store Unicode encoded as UTF-16LE, the native character type on Windows
    //> operating systems. The wide character versions of the Universal C Runtime (UCRT)
    //> library functions use wchar_t and its pointer and array types as parameters and
    //> return values, as do the wide character versions of the native Windows API.
    //
    // https://stackoverflow.com/questions/53293159/are-wchar-t-and-char16-t-the-same-thing-on-windows
    // Is wchar_t always UTF-16LE regardless of the endianness of the architecture?
    // NOT SURE and help needed. The following code assume the endianness of wchar_t architecture-specific.

    // https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding
    struct Encodings {
        int codepage;
        const char* name;
        const char* bom;
    } encodings[] = {
        { -((const char*)L"\x1")[1], "utf16le", "\xFF\xFE" },
        { -((const char*)L"\x1")[0], "utf16be", "\xFE\xFF" },
        { GetACP(), "gbk", "" },
        { 54936, "gb18030", "\x84\x31\x95\x33" },
        { CP_UTF8, "utf8", "\xEF\xBB\xBF" },
    };

    for (int i = 0; i < ARRAYSIZE(encodings); ++i)
    {
        char data[100];
        int len;
        
        if (encodings[i].codepage <= 0)
        {
            len = sizeof(words) - sizeof(words[0]);
            memcpy(data, words, len);
            if (encodings[i].codepage < 0)  // swap byte order
            {
                for (int j = 0; j < len; j += 2)
                {
                    char c = data[j];
                    data[j] = data[j+1];
                    data[j+1] = c;
                }
            }
        }
        else
        {
            len = WideCharToMultiByte(encodings[i].codepage, 0, words,
                      ARRAYSIZE(words) - 1, data, sizeof(data), NULL, NULL);
        }

        struct Info {
            const char* suffix;
            const char* bom;
        } info[3];

        if (encodings[i].bom[0])
        {
            info[0].suffix = "-bom";
            info[0].bom = encodings[i].bom;
            info[1].suffix = "-nobom";
            info[1].bom = "";
        }
        else
        {
            info[0].suffix = "";
            info[0].bom = encodings[i].bom;
            info[1].suffix = NULL;
            info[1].bom = NULL;
        }

        info[2].suffix = NULL;
        info[2].bom = NULL;

        for (const struct Info* in = info; in->suffix; ++in)
        {
            FILE* file;
            char filename[100];
            sprintf(filename, "word-%s%s.txt", encodings[i].name, in->suffix);
            file = fopen(filename, "wb");
            fwrite(in->bom, 1, strlen(in->bom), file);
            fwrite(data, 1, len, file);
            fclose(file);
        }
    }
}
