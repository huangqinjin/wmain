#include <stdio.h>
#include <windows.h>

#ifndef CP
#define CP CP_UTF8
#endif

int main(int argc, char* argv[])
{
    UINT cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP);

    printf("ANSI Code Page = %d\n", GetACP());
    printf("OEM  Code Page = %d\n", GetOEMCP());
    printf("Old Console CP = %d\n", cp);
    printf("New Console CP = %d\n", CP);

    int i = 0;
    for (char** arg = argv; *arg; ++arg)
        printf("argv[%d] = \"%s\"\n", i++, *arg);

    SetConsoleOutputCP(cp);
    return argc;
}
