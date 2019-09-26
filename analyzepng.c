#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

static double pretty_filesize_amount(unsigned long long bytes)
{
    if(bytes < (1024 * 1024))
        return bytes / 1024.0;

    if(bytes < (1024 * 1024 * 1024))
        return bytes / (1024.0 * 1024.0);

    return bytes / (1024.0 * 1024.0 * 1024.0);
}

static const char * pretty_filesize_unit(unsigned long long bytes)
{
    if(bytes < (1024 * 1024))
        return "KiB";

    if(bytes < (1024 * 1024 * 1024))
        return "MiB";

    return "GiB";
}

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
    fprintf(stderr, "%s - print information about chunks of given png files\n", argv0);
    fprintf(stderr, "Usage: %s file.png...\n", argv0);
    return 1;
}

struct myruntime
{
    jmp_buf jumper;
    FILE * f;
    unsigned long long chunks;
    unsigned long long bytes;
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

static void skip_step_int(struct myruntime * runtime, int amount)
{
    if(amount == 0)
        return;

    if(fseek(runtime->f, amount, SEEK_CUR))
    {
        fprintf(stderr, "Error: fseek(f, %d, SEEK_CUR) failed\n", amount);
        longjmp(runtime->jumper, 1);
    }

    runtime->bytes += amount;
}

static void skip(struct myruntime * runtime, unsigned amount)
{
    const int step = 1900000000;
    while(amount >= (unsigned)step)
    {
        skip_step_int(runtime, step);
        amount -= step;
    }
    skip_step_int(runtime, (int)amount);
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

static int valid_bitdepth_and_colortype(int bitdepth, int colortype)
{
    /* from png spec list of valid combinations */
    switch(colortype)
    {
        case 0:
            return bitdepth == 1 || bitdepth == 2 || bitdepth == 4 || bitdepth == 8 || bitdepth == 16;
        case 2:
            return bitdepth == 8 || bitdepth == 16;
        case 3:
            return bitdepth == 1 || bitdepth == 2 || bitdepth == 4 || bitdepth == 8;
        case 4:
            return bitdepth == 8 || bitdepth == 16;
        case 6:
            return bitdepth == 8 || bitdepth == 16;
    }
    return 0;
}

static void pretty_print_ihdr(const char * ihdrdata13)
{
    unsigned w, h;
    int bitdepth, colortype, compressionmethod, filtermethod, interlacemethod;

    /* TODO: warn if these don't fit in 31 bits, as spec says? */
    w = big_u32(ihdrdata13 + 0);
    h = big_u32(ihdrdata13 + 4);

    bitdepth = (unsigned char)ihdrdata13[8 + 0];
    colortype = (unsigned char)ihdrdata13[8 + 1];
    compressionmethod = (unsigned char)ihdrdata13[8 + 2];
    filtermethod = (unsigned char)ihdrdata13[8 + 3];
    interlacemethod = (unsigned char)ihdrdata13[8 + 4];

    /* always display dimensions and bit depth */
    printf("IHDR, 13 bytes at 16, %u x %u, %d-bit ", w, h, bitdepth);

    /* pretty print color format */
    switch(colortype)
    {
        case 0:
            printf("greyscale");
            break;
        case 2:
            printf("RGB");
            break;
        case 3:
            printf("paletted");
            break;
        case 4:
            printf("greyscale with alpha");
            break;
        case 6:
            printf("RGBA");
            break;
        default:
            printf("unknown-colortype-%d", colortype);
            break;
    } /* switch colortype */

    /* nothing printed = valid combo */
    if(!valid_bitdepth_and_colortype(bitdepth, colortype))
        printf(" (invalid bitdepth (%d) + colortype (%d) combination)", bitdepth, colortype);

    if(interlacemethod == 1)
        printf(", Adam7 interlaced");

    /* nothing printed = the valid/only compression - deflate 32 KiB */
    if(compressionmethod != 0)
        printf(", unknown compression method %d", compressionmethod);

    if(filtermethod != 0)
        printf(", unknown filter method %d", filtermethod);

    if(interlacemethod != 0 && interlacemethod != 1)
        printf(", unknown interlace method %d", interlacemethod);

    printf("\n");
}

static void verify_png_header_and_ihdr(struct myruntime * runtime)
{
    char buff[50];
    unsigned len;

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
    pretty_print_ihdr(buff + 8 + 8);
    ++runtime->chunks;
}

static int letter(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static void print_4cc_no_newline(const char * id)
{
    if(letter(id[0]) && letter(id[1]) && letter(id[2]) && letter(id[3]))
    {
        printf("%s", id);
    }
    else
    {
        printf("0x%08x", big_u32(id));
    }
}

static void print_4cc_extra_info(const char * id)
{
    if(0 == strcmp(id, "acTL"))
        printf(", APNG animation control");

    if(0 == strcmp(id, "fcTL"))
        printf(", APNG frame control");

    if(0 == strcmp(id, "fdAT"))
        printf(", APNG frame data");
}

static int parse_png_chunk(struct myruntime * runtime)
{
    char buff[10];
    unsigned len;
    read(runtime, buff, 8);
    buff[8] = '\0';
    check(runtime, 0 != strncmp(buff + 4, "IHDR", 4), "duplicate IHDR");
    len = big_u32(buff);
    print_4cc_no_newline(buff + 4);
    printf(", %u bytes at %llu", len, runtime->bytes);
    print_4cc_extra_info(buff + 4);
    printf("\n");
    skip(runtime, len);
    skip(runtime, 4); /* skip 4 byte crc that's after the chunk */
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
        fprintf(stderr, "Error: over %u bytes (%.3f %s) of trailing data\n",
            (unsigned)MAX_TRAILING_DATA_READ,
            pretty_filesize_amount(MAX_TRAILING_DATA_READ),
            pretty_filesize_unit(MAX_TRAILING_DATA_READ)
        );
        longjmp(runtime->jumper, 1);
    }

    if(loaded > 0)
    {
        fprintf(stderr, "Error: %u bytes (%.3f %s) of trailing data\n",
            loaded,
            pretty_filesize_amount(loaded),
            pretty_filesize_unit(loaded)
        );
        longjmp(runtime->jumper, 1);
    }
}

static void doit(struct myruntime * runtime)
{
    verify_png_header_and_ihdr(runtime);
    while(parse_png_chunk(runtime));
    printf("This PNG has: %llu chunks, %llu bytes (%.3f %s)\n",
        runtime->chunks, runtime->bytes,
        pretty_filesize_amount(runtime->bytes),
        pretty_filesize_unit(runtime->bytes)
    );

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
    wchar_t * fname16 = (wchar_t*)calloc(utf16chars, sizeof(wchar_t));
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

static int handle_file(const char * fname)
{
    FILE * f = my_utf8_fopen_rb(fname);
    if(f)
    {
        printf("File '%s'\n", fname);
        return parse_and_close_png_file(f);
    }

    fprintf(stderr, "Error: fopen('%s') = NULL\n", fname);
    return 1;
}

static int my_utf8_main(int argc, char ** argv)
{
    int i, anyerrs;

    if(argc < 2)
        return print_usage(argv[0]);

    anyerrs = 0;
    for(i = 1; i < argc; ++i)
    {
        if(i > 1)
            printf("\n");

        anyerrs += handle_file(argv[i]);
    } /* for */

    return !!anyerrs;
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
        utf8argv[i] = (char*)calloc(utf8len, 1);
        if(!utf8argv[i])
        {
            retcode = 1;
            fputs("Error: calloc error in wmain\n", stderr);
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
