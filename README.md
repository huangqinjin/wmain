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

## Encoding of Windows Console
[Windows Console (conhost.exe)](https://devblogs.microsoft.com/commandline/windows-command-line-inside-the-windows-console)
is a Win32 GUI app that consists of:
  - InputBuffer: Stores keyboard and mouse event records generated by user input.
  - OutputBuffer: Stores the text rendered on the Console's window client area.
  
OutputBuffer was ssentially a 2D array of [`CHAR_INFO`](https://docs.microsoft.com/en-us/windows/console/char-info-str)
structs which contain each cell's character data & attributes. That means only UCS-2 text was supported. Since Windows 10
October 2018 Update (Version 1809, Build Number 10.0.17763), [a new OutputBuffer is introduced to fully support all unicode
characters](https://devblogs.microsoft.com/commandline/windows-command-line-unicode-and-utf-8-output-text-buffer). 

Another issue is that Console uses GDI for text rendering, which doesn't support font-fallback. So some complex glyphs 
can't be displayed even if the OutputBuffer could store them.
[ConPTY](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty)
is introduced together with the new OutputBuffer. Then Console becomes a true "Console Host", which is windowless and not
responsible for user input and rendering, supporting all Command-Line apps and/or GUI apps that communicate with Command-Line
apps through [Console Virtual Terminal Sequences](https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences).
[Terminal (TTY)](https://devblogs.microsoft.com/commandline/windows-command-line-backgrounder) is such a typical GUI app
responsible for user input and rendering. With ConPTY infrastructure,
[Windows Terminal](https://github.com/microsoft/terminal) uses a new rendering engine that supports font-fallback and 
displays all testing characters correctly.

Command-Line apps use `WriteConsoleW` to write unicode text to OutputBuffer and `ReadConsoleW` to read unicode text from
InputBuffer. `WriteConsoleA/WriteFile` can also be used for output but that involves a encoding conversion from
`ConsoleOutputCP` (defaults to [OEMCP](https://docs.microsoft.com/en-us/windows/desktop/intl/code-pages)) to Unicode before
storing text into OutputBuffer. Accordingly, use `ReadConsoleA/ReadFile` for input will do the conversion from Unicode to
`ConsoleInputCP` (also defaults to OEMCP). Note that `ConsoleInputCP` only supports DBCS, see
[ms-terminal/src/host/dbcs.cpp#TranslateUnicodeToOem](https://github.com/microsoft/terminal/blob/master/src/host/dbcs.cpp).

The builtin command [`type`](https://stackoverflow.com/questions/1259084/what-encoding-code-page-is-cmd-exe-using) of the
"Command Prompt" shell (cmd.exe) checks the start of a file for a UTF-16LE BOM. If it finds such a mark, it displays the
file content using `WriteConsoleW`, otherwise using `WriteConsoleA/WriteFile`. So `type` displays correctly only for
UTF-16LE BOM-ed files and those encoded in current `ConsoleOutputCP`. In PowerShell, `type` detects BOM for UTF-16 and
UTF-8. To verify these, just run `type words\word-*.txt` in Cmd and PowerShell.
