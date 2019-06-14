## Setup environment
```
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
```

## source-charset and execution-charset
The [source-charset](https://docs.microsoft.com/en-us/cpp/build/reference/source-charset-set-source-character-set#remarks)
is the encoding used by Visual Studio to interpret the source files into the internal representation. Specially,
for [Narrow String Literals](https://docs.microsoft.com/en-us/cpp/cpp/string-and-character-literals-cpp#narrow-string-literals)
in the source files, the compiler use UTF-8 (why not UTF-16?) encoded strings as the internal representation, and then
these strings are converted to the
[execution-charset](https://docs.microsoft.com/en-us/cpp/build/reference/execution-charset-set-execution-character-set#remarks)
and store in the compiled object files.

To sum up, the compiler converts narrow string literals in source files from source-charset to Unicode and then to
execution-charset, and finally stores the results into compiled binaries. source-charset must be the encoding of the
source files used to store on disk. execution-charset is the encoding of `const char[]` in memory when the program runs.
source-charset and execution-charset are independent. If a character in the source file cannot be represented in the
execution character set, the Unicode conversion substitutes a question mark '?' character, see 
[`/validate-charset`](https://docs.microsoft.com/en-us/cpp/build/reference/validate-charset-validate-for-compatible-characters#remarks) option.

By default, execution-charset is the Windows code page, a.k.a. ANSI code page (ACP), unless you have specified a character
set name or code page by using the `/execution-charset` option. For source-charset, if no `/source-charset` option is
specified, Visual Studio detects BOM to determine if a source file is in an encoded Unicode format, for example, UTF-16
or UTF-8. If no BOM is found, it assumes the source file is encoded using ACP.

The testing source file [test\execution_charset.c](test/execution_charset.c)
is encoded as [Windows-1252](https://en.wikipedia.org/wiki/Windows-1252#Character_set) which cannot be auto-detected and
these are characters invalid in ACP. Without `/source-charset`, the compiler performs ACP to Unicode conversion for
Windows-1252 strings and complains [C4819](https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4819)
for some invalid ACP characters.
```
cl /c test\execution_charset.c

warning C4819: The file contains a character that cannot be represented in the current code page (936). Save the file in
Unicode format to prevent data loss.
```
Tell compiler the real encoding of the source file, the Unicode to ACP conversion is finally performed and the compiler
complains [C4566](https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4566)
for some Unicode characters for which then substitutes a question mark '?'. 
```
cl /c /source-charset:.1252 test\execution_charset.c

warning C4566: character represented by universal-character-name '\u00FF' cannot be represented in the current code page (936).
```
