#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

static const char * filepath_to_filename(const char * path)
{
    size_t i, len, lastslash;

    len = strlen(path);
    lastslash = 0;
    for(i = 0; i < len; ++i)
        if(path[i] == '/' || path[i] == '\\')
            lastslash = i + 1;

    return path + lastslash;
}

static int print_usage(const char * argv0)
{
    argv0 = filepath_to_filename(argv0);
    fprintf(stderr, "%s - print information about chunks of a given png file\n", argv0);
    fprintf(stderr, "Usage: %s file.png\n", argv0);
    return 1;
}

struct myruntime
{
    jmp_buf jumper;
    FILE * f;
    int chunks;
    unsigned bytes;
};

static void read(struct myruntime * runtime, void * buff, int want)
{
    const int got = fread(buff, 1, want, runtime->f);
    if(got != want)
    {
        fprintf(stderr, "Error: fread(buff, 1, %d, f) = %d\n", want, got);
        longjmp(runtime->jumper, 1);
    }
    runtime->bytes += want;
}

static void skip(struct myruntime * runtime, int amount)
{
    if(fseek(runtime->f, amount, SEEK_CUR))
    {
        fprintf(stderr, "Error: fseek(f, %d, SEEK_CUR) failed\n", amount);
        longjmp(runtime->jumper, 1);
    }
    runtime->bytes += amount;
}

static void check(struct myruntime * runtime, int b, const char * errmsg)
{
    if(!b)
    {
        fprintf(stderr, "Error: %s\n", errmsg);
        longjmp(runtime->jumper, 1);
    }
}

static unsigned big_u32(const char * buff)
{
    unsigned ret = 0u;
    const unsigned char * b = (const unsigned char *)buff;

    ret = (ret | b[0]) << 8;
    ret = (ret | b[1]) << 8;
    ret = (ret | b[2]) << 8;
    ret = (ret | b[3]);

    return ret;
}

const unsigned char kPngHeaderGood[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
const unsigned char kPngHeaderTopBitZeroed[8] = {0x09, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

const unsigned char kPngHeaderDos2Unix[7] = {0x89, 'P', 'N', 'G', '\n', 0x1A, '\n'};

const unsigned char kPngHeaderUnix2Dos[9] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\r', '\n'};
const unsigned char kPngHeaderUnix2DosBad[10] = {0x89, 'P', 'N', 'G', '\r', '\r', '\n', 0x1A, '\r', '\n'};


static void verify_png_header_and_ihdr(struct myruntime * runtime)
{
    char buff[50];
    unsigned len, w, h;

    /* 8 for png header, 8 for len and id and 13 for data of IHDR */
    read(runtime, buff, 8 + 8 + 13);

    /* check png header itself for common errors */
    check(runtime, 0 != memcmp(buff, kPngHeaderTopBitZeroed, 8), "PNG 8-byte header top bit zeroed");
    check(runtime, 0 != memcmp(buff, kPngHeaderDos2Unix, 7), "PNG 8-byte header looks like after dos2unix");
    check(runtime, 0 != memcmp(buff, kPngHeaderUnix2Dos, 9), "PNG 8-byte header looks like after unix2dos");
    check(runtime, 0 != memcmp(buff, kPngHeaderUnix2DosBad, 10), "PNG 8-byte header looks like after bad \\n to \\r\\n conversion");
    check(runtime, 0 == memcmp(buff, kPngHeaderGood, 8), "PNG 8-byte header has unknown wrong values");

    /* now check IHDR */
    check(runtime, 0 == strncmp(buff + 8 + 4, "IHDR", 4) , "first chunk isn't IHDR");
    len = big_u32(buff + 8);
    check(runtime, len == 13u , "IHDR length isn't 13");
    skip(runtime, 4); /* skip over CRC of IHDR */
    w = big_u32(buff + 8 + 8 + 0);
    h = big_u32(buff + 8 + 8 + 4);
    printf("IHDR, 13 bytes at 16, %u x %u\n", w, h);
    ++runtime->chunks;
}

static int parse_png_chunk(struct myruntime * runtime)
{
    char buff[10];
    unsigned len;
    read(runtime, buff, 8);
    buff[8] = '\0';
    check(runtime, 0 != strncmp(buff + 4, "IHDR", 4), "duplicate IHDR");
    len = big_u32(buff);
    check(runtime, len < 2000000000, "chunk len >= 2000000000, assuming corruption");
    printf("%s, %u bytes at %u\n", buff + 4, len, runtime->bytes);
    skip(runtime, (int)(len + 4u));
    ++runtime->chunks;
    return 0 != strncmp(buff + 4, "IEND", 4);
}

#define MAX_TRAILING_DATA_READ (15 * 1024 * 1024)

static void check_for_trailing_data(struct myruntime * runtime)
{
    char buff[128];
    unsigned loaded = 0u;

    while(!feof(runtime->f))
    {
        if(loaded > MAX_TRAILING_DATA_READ)
            break;

        loaded += fread(buff, 1, 128, runtime->f);
    }

    if(loaded > MAX_TRAILING_DATA_READ)
    {
        fprintf(stderr, "Error: over %u bytes of trailing data\n", (unsigned)MAX_TRAILING_DATA_READ);
        longjmp(runtime->jumper, 1);
    }

    if(loaded > 0)
    {
        fprintf(stderr, "Error: %u bytes of trailing data\n", loaded);
        longjmp(runtime->jumper, 1);
    }
}

static void doit(struct myruntime * runtime)
{
    verify_png_header_and_ihdr(runtime);
    while(parse_png_chunk(runtime));
    printf("This PNG has: %d chunks, %u bytes\n", runtime->chunks, runtime->bytes);
    check_for_trailing_data(runtime);
}

#ifdef _MSC_VER
/*for MultiByteToWideChar */
#include <Windows.h>
#endif

static FILE * my_utf8_fopen_rb(const char * fname)
{
#ifndef _MSC_VER
    return fopen(fname, "rb");
#else
    FILE * ret;
    const int utf16chars = strlen(fname) + 5;
    wchar_t * fname16 = (wchar_t*)malloc(sizeof(wchar_t) * utf16chars);
    if(!fname16)
        return NULL;

    MultiByteToWideChar(CP_UTF8, 0, fname, -1, fname16, utf16chars);
    ret = _wfopen(fname16, L"rb");
    free(fname16);
    return ret;
#endif
}

static int parse_and_close_png_file(FILE * f)
{
    struct myruntime runtime;
    memset(&runtime, 0x0, sizeof(struct myruntime));
    runtime.f = f;
    if(setjmp(runtime.jumper) == 0)
    {
        doit(&runtime);
        fclose(runtime.f);
        return 0;
    }
    else
    {
        fclose(runtime.f);
        return 1;
    }
}

static int my_utf8_main(int argc, char ** argv)
{
    FILE * f = NULL;
    if(argc != 2)
        return print_usage(argv[0]);

    f = my_utf8_fopen_rb(argv[1]);
    if(!f)
    {
        fprintf(stderr, "Error: fopen('%s') = NULL\n", argv[1]);
        return 1;
    }

    printf("File '%s'\n", argv[1]);
    return parse_and_close_png_file(f);
}

#ifndef _MSC_VER

int main(int argc, char ** argv)
{
    return my_utf8_main(argc, argv);
}

#else

/* for wcslen and WideCharToMultiByte */
#include <wchar.h>
#include <Windows.h>

int wmain(int argc, wchar_t ** argv)
{
    int i, retcode;
    char ** utf8argv = (char **)calloc(argc + 1, sizeof(char*));
    if(!utf8argv)
    {
        fputs("Error: calloc error in wmain\n", stderr);
        return 1;
    }

    retcode = 0;
    for(i = 0; i < argc; ++i)
    {
        const size_t utf8len = wcslen(argv[i]) * 3 + 10;
        utf8argv[i] = (char*)malloc(utf8len);
        if(!utf8argv[i])
        {
            retcode = 1;
            fputs("Error: malloc error in wmain\n", stderr);
            break;
        }
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8argv[i], utf8len, NULL, NULL);
    }

    if(retcode == 0)
        retcode = my_utf8_main(argc, utf8argv);

    for(i = 0; i < argc; ++i)
        free(utf8argv[i]);

    free(utf8argv);
    return retcode;
}

#endif /* _MSC_VER */
