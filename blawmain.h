#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#define BLA_WMAIN_USE_WMAIN
#endif

#if defined(BLA_WMAIN_FUNC3) && defined(BLA_WMAIN_FUNC)
#error "both BLA_WMAIN_FUNC3 and BLA_WMAIN_FUNC defined"
#endif

#if !(defined(BLA_WMAIN_FUNC3) || defined(BLA_WMAIN_FUNC))
#error "BLA_WMAIN_FUNC or BLA_WMAIN_FUNC3 must be defined"
#endif

/*

https://github.com/FRex/blawmain

BLA_WMAIN_FUNC - same as normal int main(int argc, char ** argv);
BLA_WMAIN_FUNC3 - like normal main but with extra argument for original argv
int main(int argc, char ** argv, wchar_t ** wargv), NULL outside windows
BLA_WMAIN_USING_WMAIN_BOOLEAN - macro defined to 1 if conversion is being done and 0 otherwise
BLA_WMAIN_FORCE_OFF - if you define this macro, no conversion will ever take place

TO USE:
1. define BLA_WMAIN_FUNC or BLA_WMAIN_FUNC3 - whichever you need, but only one of them (this is enforced by the preprocessor)
2. include this header in some .c or .cpp file, usually (but its not mandatory) the one with your main
3. allow the main or wmain from this file to be the entry point, and it will convert (if needed) and call your main
4. if you want to force the conversion to be off you can define BLA_WMAIN_FORCE_OFF

If the UTF-16 to UTF-8 conversion is being used then BLA_WMAIN_USING_WMAIN_BOOLEAN will be defined to 1, and
the third argument to BLA_WMAIN_FUNC3 will not be NULL, otherwise (Linux or in case of a problem on Windows)
BLA_WMAIN_USING_WMAIN_BOOLEAN is 0 and third argument to BLA_WMAIN_FUNC3 is NULL.

The converted argv is allocated in a single area, using malloc, and you are free to
change its contents, set pointers to other strings, to NULL, change string contents, etc.

It will be freed once your main returns, so tools like ASAN and valgrind will not report it as a leak. If the malloc at
the start of the program fails (extremely unlikely) the return code is 111 and an error is printed on stderr.

Conversion is done using WinAPI's WideCharToMultiByte so the results may differ by Windows version.

If no conversion is done then argv is passed as is, no allocations. Your mains return (a single int
or something that is implicitly convertible to an int) is forwarded to the OS.

NOTE: the order of 1. and 2. matters, you must define your macro before including this file, so it can use it
NOTE: in case of error "undefined reference to `WinMain'" with GCC on Windows make sure you pass -municode
NOTE: see test.c for example usage

*/

#ifdef BLA_WMAIN_FORCE_OFF
#ifdef BLA_WMAIN_USE_WMAIN /* dont undef if its not defined, to not get a warning */
#undef BLA_WMAIN_USE_WMAIN
#endif
#endif

#ifdef BLA_WMAIN_USE_WMAIN

#define BLA_WMAIN_USING_WMAIN_BOOLEAN 1

/* for WideCharToMultiByte and malloc */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

int wmain(int argc, wchar_t ** argv)
{
    int i, retcode, bytesneeded;
    char ** utf8argv;
    char * utf8ptrout;
    char * endptr;

    bytesneeded = sizeof(char*) * (argc + 1);
    for(i = 0; i < argc; ++i)
        bytesneeded += WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);

    utf8argv  = (char **)malloc(bytesneeded);

    if(!utf8argv)
    {
        fputs("Error: malloc error in wmain\n", stderr);
        return 111;
    }

    utf8argv[argc] = NULL;
    endptr = ((char*)utf8argv) + bytesneeded;
    utf8ptrout = (char*)(utf8argv + (argc + 1));
    for(i = 0; i < argc; ++i)
    {
        const int out = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8ptrout, endptr - utf8ptrout, NULL, NULL);
        utf8argv[i] = utf8ptrout;
        utf8ptrout += out;
    } /* for */

#ifdef BLA_WMAIN_FUNC3
    retcode = BLA_WMAIN_FUNC3(argc, utf8argv, argv);
#else
    retcode = BLA_WMAIN_FUNC(argc, utf8argv);
#endif

    free(utf8argv);
    return retcode;
}

int main(void)
{
    int ret, argc = 0;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    ret = wmain(argc, argv);
    LocalFree(argv);
    return ret;
}

#else

#define BLA_WMAIN_USING_WMAIN_BOOLEAN 0

int main(int argc, char ** argv)
{
#ifdef BLA_WMAIN_FUNC3
    return BLA_WMAIN_FUNC3(argc, argv, NULL);
#else
    return BLA_WMAIN_FUNC(argc, argv);
#endif
}

#endif
