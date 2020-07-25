#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

static int my_utf8_main(int argc, char ** argv);
#define BLA_WMAIN_FUNC my_utf8_main
#include "blawmain.h"

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
    fprintf(stderr, "Info : BLA_WMAIN_USING_WMAIN_BOOLEAN = %d\n", BLA_WMAIN_USING_WMAIN_BOOLEAN);
    fprintf(stderr, "Usage: %s [--no-idat] file.png...\n", argv0);
    fprintf(stderr, "    --no-idat #don't print IDAT chunk locations and sizes, can be anywhere\n");
    return 1;
}

struct myruntime
{
    jmp_buf jumper;
    FILE * f;
    int skipidat;
    unsigned long long chunks;
    unsigned long long idatchunks;
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
const unsigned char kMngHeaderGood[8] = {0x8A, 'M', 'N', 'G', '\r', '\n', 0x1A, '\n'};
const unsigned char kJngHeaderGood[8] = {0x8B, 'J', 'N', 'G', '\r', '\n', 0x1A, '\n'};

const unsigned char kPngHeaderTopBitZeroed[8] = {0x09, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
const unsigned char kMngHeaderTopBitZeroed[8] = {0x0A, 'M', 'N', 'G', '\r', '\n', 0x1A, '\n'};
const unsigned char kJngHeaderTopBitZeroed[8] = {0x0B, 'J', 'N', 'G', '\r', '\n', 0x1A, '\n'};

const unsigned char kPngHeaderDos2Unix[7] = {0x89, 'P', 'N', 'G', '\n', 0x1A, '\n'};
const unsigned char kMngHeaderDos2Unix[7] = {0x8A, 'M', 'N', 'G', '\n', 0x1A, '\n'};
const unsigned char kJngHeaderDos2Unix[7] = {0x8B, 'J', 'N', 'G', '\n', 0x1A, '\n'};

const unsigned char kPngHeaderUnix2Dos[9] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\r', '\n'};
const unsigned char kMngHeaderUnix2Dos[9] = {0x8A, 'M', 'N', 'G', '\r', '\n', 0x1A, '\r', '\n'};
const unsigned char kJngHeaderUnix2Dos[9] = {0x8B, 'J', 'N', 'G', '\r', '\n', 0x1A, '\r', '\n'};

const unsigned char kPngHeaderUnix2DosBad[10] = {0x89, 'P', 'N', 'G', '\r', '\r', '\n', 0x1A, '\r', '\n'};
const unsigned char kMngHeaderUnix2DosBad[10] = {0x8A, 'M', 'N', 'G', '\r', '\r', '\n', 0x1A, '\r', '\n'};
const unsigned char kJngHeaderUnix2DosBad[10] = {0x8B, 'J', 'N', 'G', '\r', '\r', '\n', 0x1A, '\r', '\n'};

const unsigned char kJpegTriByteStart[3] = {0xff, 0xd8, 0xff};

static void check_png_header(struct myruntime * runtime, const char * buff)
{
    /* common mangling errors that png header catches */
    check(runtime, 0 != memcmp(buff, kPngHeaderTopBitZeroed, 8), "PNG 8-byte header top bit zeroed");
    check(runtime, 0 != memcmp(buff, kPngHeaderDos2Unix, 7), "PNG 8-byte header looks like after dos2unix");
    check(runtime, 0 != memcmp(buff, kPngHeaderUnix2Dos, 9), "PNG 8-byte header looks like after unix2dos");
    check(runtime, 0 != memcmp(buff, kPngHeaderUnix2DosBad, 10), "PNG 8-byte header looks like after bad \\n to \\r\\n conversion");

    /* common mangling errors that mng header catches */
    check(runtime, 0 != memcmp(buff, kMngHeaderTopBitZeroed, 8), "MNG 8-byte header top bit zeroed, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kMngHeaderDos2Unix, 7), "MNG 8-byte header looks like after dos2unix, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kMngHeaderUnix2Dos, 9), "MNG 8-byte header looks like after unix2dos, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kMngHeaderUnix2DosBad, 10), "MNG 8-byte header looks like after bad \\n to \\r\\n conversion, this tool only handles PNG");

    /* if valid mng header print that this program is for png only */
    check(runtime, 0 != memcmp(buff, kMngHeaderGood, 8), "MNG 8-byte header valid, this tool only handles PNG");

    /* common mangling errors that jng header catches */
    check(runtime, 0 != memcmp(buff, kJngHeaderTopBitZeroed, 8), "JNG 8-byte header top bit zeroed, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kJngHeaderDos2Unix, 7), "JNG 8-byte header looks like after dos2unix, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kJngHeaderUnix2Dos, 9), "JNG 8-byte header looks like after unix2dos, this tool only handles PNG");
    check(runtime, 0 != memcmp(buff, kJngHeaderUnix2DosBad, 10), "JNG 8-byte header looks like after bad \\n to \\r\\n conversion, this tool only handles PNG");

    /* if valid jng header print that this program is not for jng */
    check(runtime, 0 != memcmp(buff, kJngHeaderGood, 8), "JNG 8-byte header valid, this tool only handles PNG");

    /* jpeg, jpg, jfif, etc. files seem to start with these 3 bytes */
    check(runtime, 0 != memcmp(buff, kJpegTriByteStart, 3), "starts with 0xff 0xd8 0xff, like a JPEG file");

    /* gif files start with ascii GIF + version, like 87a, 89a, so just look at first 3 bytes */
    check(runtime, 0 != memcmp(buff, "GIF", 3), "starts with 'GIF', like a GIF file");

    /* webp file which is in a riff container */
    check(runtime, (0 != memcmp(buff, "RIFF", 4)) || (0 != memcmp(buff + 8, "WEBP", 4)),
        "starts with 'RIFF' and has 'WEBP' on offset 8, like a WEBP file");

    /* any riff file other than webp */
    check(runtime, 0 != memcmp(buff, "RIFF", 4), "starts with 'RIFF', but has no 'WEBP' at offset 8");

    /* bmp files starts with BM + length, etc. not BMP! */
    check(runtime, 0 != memcmp(buff, "BM", 2), "starts with 'BM', like a BMP file");

    /* catches all other unknown errors so keep last */
    check(runtime, 0 == memcmp(buff, kPngHeaderGood, 8), "PNG 8-byte header has unknown wrong values");
}

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

    /* check png header itself for common errors and if its mng or jng instead */
    check_png_header(runtime, buff);

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

static void print_escaped_binary_string(const char * str, unsigned len)
{
    unsigned char c;
    unsigned i;

    for(i = 0; i < len; ++i)
    {
        c = (unsigned char)str[i];
        switch(c)
        {
            case '\\':
                fputs("\\", stdout);
                break;
            case '\0':
                fputs("\\0", stdout);
                break;
            default:
                if(' ' <= c && c <= '~')
                    putchar(c);
                else
                    printf("\\x%x%x", c >> 4, c & 0xf);
                break;
        } /* switch c */
    } /* for i */
}

#define MAX_TEXT_BUFF 512

static unsigned print_extra_info(struct myruntime * runtime, unsigned len, const char * id)
{
    if(0 == strcmp(id, "tEXt"))
    {
        char buff[MAX_TEXT_BUFF];
        int truncated;

        truncated = len > MAX_TEXT_BUFF;
        if(truncated)
            len = MAX_TEXT_BUFF;

        read(runtime, buff, len);
        if(truncated)
            fputs(", part, ", stdout);
        else
            fputs(", full, ", stdout);

        print_escaped_binary_string(buff, len);
        return len;
    } /* if tEXt */

    if(0 == strcmp(id, "pHYs"))
    {
        char buff[9];
        unsigned x, y;

        if(len != 9u)
        {
            fputs(", not 9 bytes so not parsed", stdout);
            return 0u;
        }

        read(runtime, buff, 9u);
        x = big_u32(buff + 0);
        y = big_u32(buff + 4);
        if(buff[8] == 1)
            printf(", %u x %u pixels per meter", x, y);
        else
            printf(", %u x %u pixels per unknown unit (%d)\n", x, y, buff[8]);

        return 9u;
    } /* if pHYs */

    if(0 == strcmp(id, "tIME"))
    {
        char buff[7];
        int year;

        if(len != 7)
        {
            fputs(", not 7 bytes so not parsed", stdout);
            return 0u;
        }

        read(runtime, buff, 7u);
        year = (unsigned char)buff[0];
        year = (year << 8) + (unsigned char)buff[1];
        printf(
            ", %d-%02d-%02d %02d:%02d:%02d",
            year, buff[2], buff[3], /* year month day */
            buff[4], buff[5], buff[6] /* hour minute second */
        );

        return 7u;
    } /* tIME */

    return 0u; /* nothing special and no extra in chunk data read */
}

static int parse_png_chunk(struct myruntime * runtime)
{
    char buff[10];
    unsigned len, usedbyextra;
    int isidat;
    read(runtime, buff, 8);
    buff[8] = '\0';
    check(runtime, 0 != strncmp(buff + 4, "IHDR", 4), "duplicate IHDR");
    isidat = (0 == strncmp(buff + 4, "IDAT", 4));
    len = big_u32(buff);
    usedbyextra = 0u; /* set to 0 in case the if doesnt run its body */

    /* print if its not idat or if its idat but we arent skipping them */
    if(!isidat || !runtime->skipidat)
    {
        print_4cc_no_newline(buff + 4);
        printf(", %u bytes at %llu", len, runtime->bytes);
        print_4cc_extra_info(buff + 4);
        usedbyextra = print_extra_info(runtime, len, buff + 4);
        printf("\n");
    }

    skip(runtime, len - usedbyextra); /* skip any data not consumed by extra info print */
    skip(runtime, 4); /* skip 4 byte crc that's after the chunk */
    ++runtime->chunks;
    runtime->idatchunks += isidat;
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
    printf("This PNG has: %llu chunks (%llu IDAT), %llu bytes (%.3f %s)\n",
        runtime->chunks, runtime->idatchunks, runtime->bytes,
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

    /* surely enough for wchars + nul, it overalloactes quite a bit though:
     * any 1-3 byte utf-8 sequence is 1 utf-16 wchar: 1-3 >= 1
     * any 4 byte utf-8 sequence is 2 utf-16 wchars: 4 > 2 */
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

/* this helper is to avoid using volatile but also not
 * have any locals in same scope as the setjmp call */
static int setjmp_and_doit(struct myruntime * runtime)
{
    if(setjmp(runtime->jumper) == 0)
    {
        doit(runtime);
        return 0;
    }

    /* if setjmp returned non-zero from longjmp return 1 aka error */
    return 1;
}

static int parse_and_close_png_file(FILE * f, int skipidat)
{
    struct myruntime runtime;
    int ret;

    memset(&runtime, 0x0, sizeof(struct myruntime));
    runtime.f = f;
    runtime.skipidat = skipidat;
    ret = setjmp_and_doit(&runtime);
    fclose(f);
    return ret;
}

static int handle_file(const char * fname, int skipidat)
{
    FILE * f = my_utf8_fopen_rb(fname);
    if(f)
    {
        printf("File '%s'\n", fname);
        return parse_and_close_png_file(f, skipidat);
    }

    fprintf(stderr, "Error: fopen('%s') = NULL\n", fname);
    return 1;
}

static int is_dash_dash_no_idat(const char * str)
{
    return 0 == strcmp("--no-idat", str);
}

static int count_dash_dash_no_idat(int argc, char ** argv)
{
    int i, ret;

    ret = 0;
    for(i = 1; i < argc; ++i)
        if(is_dash_dash_no_idat(argv[i]))
            ++ret;

    return ret;
}

static int my_utf8_main(int argc, char ** argv)
{
    int i, anyerrs, skipidat, files;

    skipidat = count_dash_dash_no_idat(argc, argv);
    if((argc - skipidat) < 2)
        return print_usage(argv[0]);

    anyerrs = 0;
    files = 0;
    for(i = 1; i < argc; ++i)
    {
        if(is_dash_dash_no_idat(argv[i]))
            continue;

        if(files > 0)
            printf("\n");

        ++files;
        anyerrs += handle_file(argv[i], !!skipidat);
    } /* for */

    return !!anyerrs;
}
