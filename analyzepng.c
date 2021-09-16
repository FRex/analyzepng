#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#define ANALYZEPNG_ON_WINDOWS
#endif
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <math.h>

#ifdef ANALYZEPNG_ON_WINDOWS
#include <fcntl.h>
#include <io.h>
#endif

static void ensureNoWindowsLineConversions(void)
{
#ifdef ANALYZEPNG_ON_WINDOWS
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif /* ANALYZEPNG_ON_WINDOWS */
}

static int my_utf8_main(int argc, char ** argv);

/* we do ifdefs around Windows already and this lets us be 1 .c file on other OSes */
#ifdef ANALYZEPNG_ON_WINDOWS
#define BLA_WMAIN_FUNC my_utf8_main
#include "blawmain.h"
#else
#define BLA_WMAIN_USING_WMAIN_BOOLEAN 0
int main(int argc, char ** argv)
{
    return my_utf8_main(argc, argv);
}
#endif

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

static int print_usage(const char * argv0, FILE * f)
{
    argv0 = filepath_to_filename(argv0);
    fprintf(f, "%s - print information about chunks of given png files\n", argv0);
    if(BLA_WMAIN_USING_WMAIN_BOOLEAN)
        fprintf(f, "Windows build capable of colors and UTF-16 filenames\n");

    fprintf(f, "Usage: %s [--no-idat] file.png...\n", argv0);
    fprintf(f, "    --h OR --help #print this help to stdout\n");
    fprintf(f, "    --no-idat #don't print IDAT chunk locations and sizes, can be anywhere\n");
    fprintf(f, "    --plte #print RGB values from the PLTE chunk\n");
    fprintf(f, "    --color-plte #print RGB values from the PLTE chunk using ANSI escape codes\n");
    fprintf(f, "    +set-bash-completion #print command to set bash completion\n");
    fprintf(f, "    +do-bash-completion #do completion based on args from bash\n");
    return 1;
}

struct myruntime
{
    jmp_buf jumper;
    FILE * f;
    int skipidat;
    int showpalette;
    int showpalettecolors;
    unsigned long long chunks;
    unsigned long long idatchunks;
    unsigned long long bytes;
    unsigned long long pixelcount;
    int bitsperpixel;
    int sbitbytes;
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

static void error(struct myruntime * runtime, const char * errmsg)
{
    fprintf(stderr, "Error: %s\n", errmsg);
    longjmp(runtime->jumper, 1);
}

/* ensures b - does nothing if its true, prints Errr: errmsg and longjmps if its false */
static void ensure(struct myruntime * runtime, int b, const char * errmsg)
{
    if(!b)
        error(runtime, errmsg);
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

static char to_printable(char c)
{
    if(' ' <= c && c <= '~')
        return c;

    return '.';
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

/* buff must be at least 10 bytes (its 8 + 8 + 13 now) since some bad headers are 9 or 10 bytes */
static void check_png_header(struct myruntime * runtime, const char * buff)
{
    /* common mangling errors that png header catches */
    ensure(runtime, 0 != memcmp(buff, kPngHeaderTopBitZeroed, 8), "PNG 8-byte header top bit zeroed");
    ensure(runtime, 0 != memcmp(buff, kPngHeaderDos2Unix, 7), "PNG 8-byte header looks like after dos2unix");
    ensure(runtime, 0 != memcmp(buff, kPngHeaderUnix2Dos, 9), "PNG 8-byte header looks like after unix2dos");
    ensure(runtime, 0 != memcmp(buff, kPngHeaderUnix2DosBad, 10), "PNG 8-byte header looks like after bad \\n to \\r\\n conversion");

    /* common mangling errors that mng header catches */
    ensure(runtime, 0 != memcmp(buff, kMngHeaderTopBitZeroed, 8), "MNG 8-byte header top bit zeroed, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kMngHeaderDos2Unix, 7), "MNG 8-byte header looks like after dos2unix, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kMngHeaderUnix2Dos, 9), "MNG 8-byte header looks like after unix2dos, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kMngHeaderUnix2DosBad, 10), "MNG 8-byte header looks like after bad \\n to \\r\\n conversion, this tool only handles PNG");

    /* if valid mng header print that this program is for png only */
    ensure(runtime, 0 != memcmp(buff, kMngHeaderGood, 8), "MNG 8-byte header valid, this tool only handles PNG");

    /* common mangling errors that jng header catches */
    ensure(runtime, 0 != memcmp(buff, kJngHeaderTopBitZeroed, 8), "JNG 8-byte header top bit zeroed, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kJngHeaderDos2Unix, 7), "JNG 8-byte header looks like after dos2unix, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kJngHeaderUnix2Dos, 9), "JNG 8-byte header looks like after unix2dos, this tool only handles PNG");
    ensure(runtime, 0 != memcmp(buff, kJngHeaderUnix2DosBad, 10), "JNG 8-byte header looks like after bad \\n to \\r\\n conversion, this tool only handles PNG");

    /* if valid jng header print that this program is not for jng */
    ensure(runtime, 0 != memcmp(buff, kJngHeaderGood, 8), "JNG 8-byte header valid, this tool only handles PNG");

    /* jpeg, jpg, jfif, etc. files seem to start with these 3 bytes */
    ensure(runtime, 0 != memcmp(buff, kJpegTriByteStart, 3), "starts with 0xff 0xd8 0xff, like a JPEG file");

    /* gif files start with ascii GIF + version, like 87a, 89a, so just look at first 3 bytes */
    ensure(runtime, 0 != memcmp(buff, "GIF", 3), "starts with 'GIF', like a GIF file");

    /* webp file which is in a riff container */
    ensure(runtime, (0 != memcmp(buff, "RIFF", 4)) || (0 != memcmp(buff + 8, "WEBP", 4)),
        "starts with 'RIFF' and has 'WEBP' on offset 8, like a WEBP file");

    /* any riff file other than webp */
    ensure(runtime, 0 != memcmp(buff, "RIFF", 4), "starts with 'RIFF', but has no 'WEBP' at offset 8");

    /* bmp files starts with BM + length, etc. not BMP! */
    ensure(runtime, 0 != memcmp(buff, "BM", 2), "starts with 'BM', like a BMP file");

    /* catches all other unknown errors so keep last */
    if(0 != memcmp(buff, kPngHeaderGood, 8))
    {
        char errmsg[200];
        int i;

        strcpy(errmsg, "PNG 8-byte header has unknown wrong values:");
        for(i = 0; i < 8; ++i)
            sprintf(strchr(errmsg, '\0'), " 0x%02x", (unsigned char)buff[i]);

        strcat(errmsg, " \"");
        for(i = 0; i < 8; ++i)
            sprintf(strchr(errmsg, '\0'), "%c", to_printable(buff[i]));

        strcat(errmsg, "\"");
        error(runtime, errmsg);
    }
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

static void pretty_print_ihdr(struct myruntime * runtime, const char * ihdrdata13)
{
    unsigned w, h;
    int bitdepth, colortype, compressionmethod, filtermethod, interlacemethod;

    /* TODO: warn if these don't fit in 31 bits, as spec says? */
    w = big_u32(ihdrdata13 + 0);
    h = big_u32(ihdrdata13 + 4);
    runtime->pixelcount = w * (unsigned long long)h;

    bitdepth = (unsigned char)ihdrdata13[8 + 0];
    colortype = (unsigned char)ihdrdata13[8 + 1];
    compressionmethod = (unsigned char)ihdrdata13[8 + 2];
    filtermethod = (unsigned char)ihdrdata13[8 + 3];
    interlacemethod = (unsigned char)ihdrdata13[8 + 4];

    /* always display dimensions and bit depth */
    printf("IHDR, 13 bytes at 16, %u x %u, %d-bit ", w, h, bitdepth);

    /* pretty print color format */
    runtime->bitsperpixel = bitdepth;
    switch(colortype)
    {
        case 0:
            printf("greyscale");
            runtime->sbitbytes = 1;
            break;
        case 2:
            printf("RGB");
            runtime->bitsperpixel *= 3;
            runtime->sbitbytes = 3;
            break;
        case 3:
            printf("paletted");
            runtime->bitsperpixel = 8 * 3; /* each palette entry is R8 G8 B8*/
            runtime->sbitbytes = 3;
            break;
        case 4:
            printf("greyscale with alpha");
            runtime->bitsperpixel *= 2;
            runtime->sbitbytes = 2;
            break;
        case 6:
            printf("RGBA");
            runtime->bitsperpixel *= 4;
            runtime->sbitbytes = 4;
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
    ensure(runtime, 0 == strncmp(buff + 8 + 4, "IHDR", 4) , "first chunk isn't IHDR");
    len = big_u32(buff + 8);
    ensure(runtime, len == 13u , "IHDR length isn't 13");
    skip(runtime, 4); /* skip over CRC of IHDR */
    pretty_print_ihdr(runtime, buff + 8 + 8); /* also sets some fields in runtime */
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

#define MAX_TEXT_BUFF 4096

static float RGBtoBrightness(const unsigned char * rgb)
{
    const float r = rgb[0];
    const float g = rgb[1];
    const float b = rgb[2];
    return sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b);
}

static int compareRGB(const void * a, const void * b)
{
    const float ha = RGBtoBrightness((const unsigned char*)a);
    const float hb = RGBtoBrightness((const unsigned char*)b);
    return (ha > hb) - (ha < hb);
}

static unsigned print_extra_info(struct myruntime * runtime, unsigned len, const char * id)
{
    if(0 == strcmp(id, "tEXt") || 0 == strcmp(id, "iTXt"))
    {
        char buff[MAX_TEXT_BUFF];
        int truncated;

        truncated = len > MAX_TEXT_BUFF;
        if(truncated)
            len = MAX_TEXT_BUFF;

        read(runtime, buff, len);

        if(0 == strcmp(id, "iTXt"))
        {
            int i, compressed, method;

            for(i = 0; i < (int)(len - 2); ++i)
            {
                if(buff[i] == '\0')
                {
                    compressed = buff[i + 1];
                    method = buff[i + 2];
                    printf(", compression flag = %d, method = %d", compressed, method);
                    break;
                }
            } /* for */
        } /* if iTXt */

        if(truncated)
            fputs(", part (as ASCII + escapes): ", stdout);
        else
            fputs(", full (as ASCII + escapes): ", stdout);

        print_escaped_binary_string(buff, len);
        return len;
    } /* if tEXt or iTXt */

    if((runtime->showpalette || runtime->showpalettecolors) && 0 == strcmp(id, "PLTE"))
    {
        unsigned char palette[1000];
        unsigned palettesize = len;
        int i, extra = 0;
        printf(", %u entries", palettesize / 3u);

        if(palettesize > 3 * 256)
        {
            printf(" (%d too many, max is 256)", (len - 3 * 256) / 3);
            extra = palettesize % 3;
            palettesize = 3 * 256;
        }

        if(extra > 0)
            printf(" and %d extraneous byte%s", extra, (extra > 1) ? "s" : "");

        printf(", sorted by brightness: ");
        read(runtime, palette, palettesize);
        qsort(palette, palettesize / 3, 3, compareRGB);

        for(i = 0; i < (int)palettesize; i += 3)
        {
            if(runtime->showpalette && i > 0)
                fputs(", ", stdout);

            if(runtime->showpalettecolors)
                printf("\033[48;2;%d;%d;%dm \033[0m", palette[i], palette[i + 1], palette[i + 2]);

            if(runtime->showpalette)
                printf("%d %d %d", palette[i], palette[i + 1], palette[i + 2]);
        }

        return palettesize;
    } /* if PLTE */

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
            printf(", %u x %u pixels per unknown unit (%d)", x, y, buff[8]);

        return 9u;
    } /* if pHYs */

    if(0 == strcmp(id, "sBIT"))
    {
        unsigned char buff[4]; /* up to 4 bytes sBIT is okay*/
        int neededbytes, i;

        neededbytes = runtime->sbitbytes;
        fputs(",", stdout);
        if(len != neededbytes)
        {
            printf(" expected %d sBIT bytes but got %u instead", neededbytes, len);
            if(len < (unsigned)neededbytes)
                neededbytes = (int)len;
        }

        read(runtime, buff, neededbytes);
        runtime->bitsperpixel = 0;
        for(i = 0; i < neededbytes; ++i)
        {
            printf(" %d", buff[i]);
            runtime->bitsperpixel += buff[i];
        }

        return neededbytes;
    } /* sBIT */

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

    if(0 == strcmp(id, "sRGB"))
    {
        unsigned char intent;

        if(len != 1)
        {
            fputs(", sRGB is not 1 byte", stdout);
            return 0u;
        }

        read(runtime, &intent, 1u);
        switch(intent)
        {
        case 0:
            fputs(", 0 - perceptual", stdout);
            break;
        case 1:
            fputs(", 1 - relative colorimetric", stdout);
            break;
        case 2:
            fputs(", 2 - saturation", stdout);
            break;
        case 3:
            fputs(", 3 - absolute colorimetric", stdout);
            break;
        default:
            printf(", %d - unknown", intent);
            break;
        }

        return 1u;
    } /* sRGB */

    return 0u; /* nothing special and no extra in chunk data read */
}

static int parse_png_chunk(struct myruntime * runtime)
{
    char buff[10];
    unsigned len, usedbyextra;
    int isidat;
    read(runtime, buff, 8);
    buff[8] = '\0';
    ensure(runtime, 0 != strncmp(buff + 4, "IHDR", 4), "duplicate IHDR");
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
    unsigned long long imagedatabytes;

    verify_png_header_and_ihdr(runtime);
    while(parse_png_chunk(runtime));

    /* calculating it here and not in IDHR in case sBIT changed the 2nd field */
    imagedatabytes = (runtime->pixelcount * runtime->bitsperpixel + 7) / 8;
    printf("This PNG has %llu chunks (%llu IDAT), %llu bytes (%.3f %s), encoding %llu bytes (%.3f %s) of image data (%.2f%%)\n",
        runtime->chunks, runtime->idatchunks,
        runtime->bytes,
        pretty_filesize_amount(runtime->bytes),
        pretty_filesize_unit(runtime->bytes),
        imagedatabytes,
        pretty_filesize_amount(imagedatabytes),
        pretty_filesize_unit(imagedatabytes),
        (100.0 * runtime->bytes) / imagedatabytes
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

static int parse_and_close_png_file(FILE * f, int skipidat, int showpalette, int showpalettecolors)
{
    struct myruntime runtime;
    int ret;

    memset(&runtime, 0x0, sizeof(struct myruntime));
    runtime.f = f;
    runtime.skipidat = skipidat;
    runtime.showpalette = showpalette;
    runtime.showpalettecolors = showpalettecolors;
    ret = setjmp_and_doit(&runtime);
    fclose(f);
    return ret;
}

static int handle_file(const char * fname, int skipidat, int showpalette, int showpalettecolors)
{
    FILE * f = my_utf8_fopen_rb(fname);
    if(f)
    {
        printf("File '%s'\n", fname);
        return parse_and_close_png_file(f, skipidat, showpalette, showpalettecolors);
    }

    fprintf(stderr, "Error: fopen('%s') = NULL\n", fname);
    return 1;
}

static int samestring(const char * a, const char * b)
{
    return 0 == strcmp(a,b);
}

static int count_exact_option_presence(int argc, char ** argv, const char * option)
{
    int i, ret;

    ret = 0;
    for(i = 1; i < argc; ++i)
        if(samestring(argv[i], option))
            ++ret;

    return ret;
}

static void fputs_with_escaped_slashes(const char * s, FILE * f)
{
    while(*s)
    {
        switch(*s)
        {
            case ' ': fputs("\\ ", f); break;
            case '\\': fputs("\\\\", f); break;
            default: fputc(*s, f); break;
        } /* switch *s */

        ++s;
    } /* while *s */
}

#define OPTION_STRINGS_COUNT 7
const char * const kAllOptionStrings[OPTION_STRINGS_COUNT] = {
    "--no-idat",
    "--plte", "--color-plte",
    "-h", "--help",
    "+set-bash-completion", "+do-bash-completion",
};

static size_t count_matching_chars(const char * a, const char * b)
{
    size_t ret = 0u;
    while(a[ret] && b[ret] && a[ret] == b[ret])
        ++ret;

    return ret;
}

static int handle_completion(int argc, char ** argv)
{
    if(count_exact_option_presence(argc, argv, "+set-bash-completion"))
    {
        if(argc != 2)
            fprintf(stderr, "warning: other arguments specified along with +set-bash-completion\n");

        printf("complete -o default -C '");
        fputs_with_escaped_slashes(argv[0], stdout);
        printf(" +do-bash-completion' %s\n", filepath_to_filename(argv[0]));
        return 1;
    } /* +set-bash-completion present */

    if(argc == 5 && samestring(argv[1], "+do-bash-completion"))
    {
        int i;
        size_t maxmatchlen, matchinglens[OPTION_STRINGS_COUNT];

        /* nothing to do, let bash use other completions instead */
        if(strlen(argv[3]) == 0)
            return 1;

        /* find lengths of all matches and then.. */
        maxmatchlen = 0;
        for(i = 0; i < OPTION_STRINGS_COUNT; ++i)
        {
            matchinglens[i] = count_matching_chars(argv[3], kAllOptionStrings[i]);
            if(matchinglens[i] > maxmatchlen)
                maxmatchlen = matchinglens[i];
        }

        /* dont print in this case, it would not allow filename completions */
        if(maxmatchlen == 0)
            return 1;

        /* ..and then print all the longest ones */
        for(i = 0; i < OPTION_STRINGS_COUNT; ++i)
            if(matchinglens[i] == maxmatchlen)
                puts(kAllOptionStrings[i]);

        return 1;
    } /* argc == 5 and argv[1] is +do-bash-completion */

    if(count_exact_option_presence(argc, argv, "+do-bash-completion"))
    {
        fprintf(stderr, "Error: wrong use or number of arguments to +do-bash-completion\n");
        print_usage(argv[0], stderr);
        return 1;
    }

    /* no +set-bash-completion or +do-bash-completion at all */
    return 0;
}

static int enableConsoleColor(void)
{
#ifndef ANALYZEPNG_ON_WINDOWS
    return 1; /* outside windows just assume it will work */
#else
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0u;

    /* using 'Console' winapi func fails if stdout isn't a tty/is redirected so
     * assume we just want to dump ANSI color sequences to file in that case */
    if(!_isatty(_fileno(stdout)))
        return 1;

    if(console == INVALID_HANDLE_VALUE)
    {
        fprintf(
            stderr,
            "GetStdHandle(STD_OUTPUT_HANDLE) returned INVALID_HANDLE_VALUE, GetLastError() = %u\n",
            (unsigned)GetLastError()
        );
        return 0;
    }

    if(console == NULL)
    {
        fprintf(stderr, "GetStdHandle(STD_OUTPUT_HANDLE) returned NULL\n");
        return 0;
    }

    if(!GetConsoleMode(console, &mode))
    {
        fprintf(stderr, "GetConsoleMode(console, &mode) returned false, GetLastError() = %u\n",
            (unsigned)GetLastError());
        return 0;
    }

    /* ENABLE_VIRTUAL_TERMINAL_PROCESSING, by value in case its missing from header... */
    mode |= 0x0004;

    if(!SetConsoleMode(console, mode))
    {
        fprintf(stderr, "SetConsoleMode(console, mode) returned false, GetLastError() = %u\n",
            (unsigned)GetLastError());
        return 0;
    }

    return 1;
#endif /* ANALYZEPNG_ON_WINDOWS */
}

static int my_utf8_main(int argc, char ** argv)
{
    int i, anyerrs, skipidat, files, showpalette, showpalettecolors;

    ensureNoWindowsLineConversions();
    if(count_exact_option_presence(argc, argv, "-h") || count_exact_option_presence(argc, argv, "--help"))
    {
        print_usage(argv[0], stdout); /* to stdout since it was requested explicitely */
        /*  make sure to return 0, not 1 from print_usage, since this is
            requested/correct printing of help, not printing it on error */
        return 0;
    } /* if -h or --help argument present */

    if(handle_completion(argc, argv))
        return 0;

    skipidat = count_exact_option_presence(argc, argv, "--no-idat");
    showpalette = count_exact_option_presence(argc, argv, "--plte");
    showpalettecolors = count_exact_option_presence(argc, argv, "--color-plte");
    if((argc - skipidat) < 2)
        return print_usage(argv[0], stderr);

    /* only enable colors if they're needed */
    if(showpalettecolors)
        enableConsoleColor();

    anyerrs = 0;
    files = 0;
    for(i = 1; i < argc; ++i)
    {
        if(samestring(argv[i], "--no-idat") || samestring(argv[i], "--plte") || samestring(argv[i], "--color-plte"))
            continue;

        if(files > 0)
            printf("\n");

        ++files;
        anyerrs += handle_file(argv[i], !!skipidat, !!showpalette, !!showpalettecolors);
    } /* for */

    return !!anyerrs;
}
