#ifdef _MSC_VER
#define BLA_WMAIN_USE_WMAIN
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

    retcode = BLA_WMAIN_FUNC(argc, utf8argv);
    free(utf8argv);
    return retcode;
}

#else

#define BLA_WMAIN_USING_WMAIN_BOOLEAN 0

int main(int argc, char ** argv)
{
    return BLA_WMAIN_FUNC(argc, argv);
}

#endif
