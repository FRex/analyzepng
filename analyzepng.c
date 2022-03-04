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
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

const unsigned kCrcTable[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static unsigned crcStart(void)
{
    return 0xffffffff;
}

static unsigned crcUpdate(unsigned crc, const void * data, int len)
{
    int i;
    const unsigned char * bytes = (const unsigned char*)data;

    for(i = 0; i < len; ++i)
    {
        const unsigned char idx = (crc ^ bytes[i]) & 0xff;
        crc = (crc >> 8) ^ kCrcTable[idx];
    } /* for i */

    return crc;
}

static unsigned crcFinish(unsigned crc)
{
    return crc ^ 0xffffffff;
}

static int samestring(const char * a, const char * b)
{
    return 0 == strcmp(a,b);
}

static int isoption(const char * arg)
{
    return samestring(arg, "--no-idat") || samestring(arg, "--plte") || samestring(arg, "--color-plte") || samestring(arg, "--crc");
}

#ifdef __OpenBSD__
#include <unistd.h>
#include <err.h>
#define OPENBSD_PLEDGE_AND_UNVEIL_USED 1
static void applyOpenBsdRestrictions(int argc, char ** argv)
{
    int i, unveiled;

    /* hide all files except the filename arguments, skip "-" too, because it means stdin */
    unveiled = 0;
    for(i = 1; i < argc; ++i)
    {
        if(isoption(argv[i]) || samestring(argv[i], "-"))
            continue;

        ++unveiled;
        if(unveil(argv[i], "r") == -1)
            err(1, "unveil");
    }

    if(unveiled == 0)
    {
        /* only allow stdio and no disk access, since we have 0 files on disk to process */
        if(pledge("stdio", NULL) == -1)
            err(1, "pledge");
    }
    else
    {
        /* only allow stdio and reading files, this also takes away ability to unveil */
        if(pledge("stdio rpath", NULL) == -1)
            err(1, "pledge");
    }
}
#else
#define OPENBSD_PLEDGE_AND_UNVEIL_USED 0
static void applyOpenBsdRestrictions(int argc, char ** argv) {}
#endif /* __OpenBSD__ */

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

    if(OPENBSD_PLEDGE_AND_UNVEIL_USED)
        fprintf(f, "OpenBSD build using pledge(2) and unveil(2) for extra safety\n");

    fprintf(f, "Usage: %s [--no-idat] file.png... # a single - means read from stdin\n", argv0);
    fprintf(f, "    --h OR --help #print this help to stdout\n");
    fprintf(f, "    --no-idat #don't print IDAT chunk locations and sizes, can be anywhere\n");
    fprintf(f, "    --plte #print RGB values from the PLTE chunk\n");
    fprintf(f, "    --color-plte #print RGB values from the PLTE chunk using ANSI escape codes\n");
    fprintf(f, "    --crc #verify CRC checksums of the chunks, slow for big files\n");
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
    unsigned long long badcrc;
    int bitsperpixel;
    int sbitbytes;
    int verifycrc;
    unsigned crcvar;
};

/* named myread to not conflict with POSIX function of same name */
static void myread(struct myruntime * runtime, void * buff, int want)
{
    const int got = fread(buff, 1, want, runtime->f);
    if(got != want)
    {
        fprintf(stderr, "Error: fread(buff, 1, %d, f) = %d, runtime->bytes = %llu\n", want, got, runtime->bytes);
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

static void error(struct myruntime * runtime, const char * errmsg)
{
    fprintf(stderr, "Error: %s\n", errmsg);
    longjmp(runtime->jumper, 1);
}

#define SKIP_THROWAWAY_BUFF_SIZE (32 * 1024 * 1024)
static char skipThrowawayBuff[SKIP_THROWAWAY_BUFF_SIZE];

static void skip(struct myruntime * runtime, unsigned amount)
{
    /* if it's stdin then read to skip since fseek doesn't work */
    if(runtime->f == stdin)
    {
        while(amount > 0)
        {
            unsigned toread = SKIP_THROWAWAY_BUFF_SIZE;
            if(amount < toread)
                toread = amount;

            if(toread != fread(skipThrowawayBuff, 1, toread, runtime->f))
                error(runtime, "fread on stdin failed or truncated");

            amount -= toread;
            runtime->bytes += toread;
        } /* while amount > 0 */

        return;
    }

    const int step = 1900000000;
    while(amount >= (unsigned)step)
    {
        skip_step_int(runtime, step);
        amount -= step;
    }
    skip_step_int(runtime, (int)amount);
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

/* pretty print CRC ok or not okay, returns 1 if they d not match */
static int printCrc(unsigned mycrc, unsigned filecrc)
{
    if(mycrc == filecrc)
        printf(", OK CRC 0x%08x", mycrc);
    else
        printf(", WRONG CRC 0x%08x (computed) 0x%08x (in file)", mycrc, filecrc);

    return mycrc != filecrc;
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

    if(runtime->verifycrc)
    {
        unsigned crc = crcStart();
        crc = crcUpdate(crc, "IHDR", 4);
        crc = crcUpdate(crc, ihdrdata13, 13);
        crc = crcFinish(crc);
        runtime->badcrc += printCrc(crc, big_u32(ihdrdata13 + 13));
    }

    printf("\n");
}

static void verify_png_header_and_ihdr(struct myruntime * runtime)
{
    char buff[50];
    unsigned len;

    /* 8 for png header, 8 for len and id and 13 + 4 for data + crc of IHDR */
    myread(runtime, buff, 8 + 8 + 13 + 4);

    /* check png header itself for common errors and if its mng or jng instead */
    check_png_header(runtime, buff);

    /* now check IHDR */
    ensure(runtime, 0 == strncmp(buff + 8 + 4, "IHDR", 4) , "first chunk isn't IHDR");
    len = big_u32(buff + 8);
    ensure(runtime, len == 13u , "IHDR length isn't 13");
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
    return sqrt(0.299 * (r * r) + 0.587 * (g * g) + 0.114 * (b * b));
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

        myread(runtime, buff, len);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, buff, len);

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
        myread(runtime, palette, palettesize);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, palette, palettesize);

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

        myread(runtime, buff, 9u);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, buff, 9u);

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

        myread(runtime, buff, neededbytes);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, buff, neededbytes);


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

        myread(runtime, buff, 7u);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, buff, 7u);

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

        myread(runtime, &intent, 1u);
        if(runtime->verifycrc)
            runtime->crcvar = crcUpdate(runtime->crcvar, &intent, 1u);

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
    int isidat, printchunk;
    myread(runtime, buff, 8);
    buff[8] = '\0';
    ensure(runtime, 0 != strncmp(buff + 4, "IHDR", 4), "duplicate IHDR");
    isidat = (0 == strncmp(buff + 4, "IDAT", 4));
    len = big_u32(buff);
    usedbyextra = 0u; /* set to 0 in case the if doesnt run its body */

    if(runtime->verifycrc)
        runtime->crcvar = crcUpdate(crcStart(), buff + 4, 4);

    /* print if its not idat or if its idat but we arent skipping them */
    printchunk = !isidat || !runtime->skipidat;
    if(printchunk)
    {
        print_4cc_no_newline(buff + 4);
        printf(", %u bytes at %llu", len, runtime->bytes);
        print_4cc_extra_info(buff + 4);
        usedbyextra = print_extra_info(runtime, len, buff + 4);
    }

    if(runtime->verifycrc)
    {
        unsigned char buff[16 * 1024];
        unsigned bytesleft = len - usedbyextra;
        while(bytesleft > 0)
        {
            const int toread = (bytesleft < 16 * 1024) ? bytesleft : (16 * 1024);
            myread(runtime, buff, toread);
            runtime->crcvar = crcUpdate(runtime->crcvar, buff, toread);
            bytesleft -= toread;
        }

        myread(runtime, buff, 4);
        runtime->crcvar = crcFinish(runtime->crcvar);
        if(printchunk)
            runtime->badcrc += printCrc(runtime->crcvar, big_u32(buff));
        else
            runtime->badcrc += runtime->crcvar != big_u32(buff);
    }
    else
    {
        skip(runtime, len - usedbyextra); /* skip any data not consumed by extra info print */
        skip(runtime, 4); /* skip 4 byte crc that's after the chunk */
    }

    if(printchunk)
        printf("\n");

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
    printf("This PNG has %llu chunks (%llu IDAT), %llu bytes (%.3f %s), encoding %llu bytes (%.3f %s) of image data (%.2f%%)",
        runtime->chunks, runtime->idatchunks,
        runtime->bytes,
        pretty_filesize_amount(runtime->bytes),
        pretty_filesize_unit(runtime->bytes),
        imagedatabytes,
        pretty_filesize_amount(imagedatabytes),
        pretty_filesize_unit(imagedatabytes),
        (100.0 * runtime->bytes) / imagedatabytes
    );

    if(runtime->verifycrc)
        printf(", %llu bad CRC", runtime->badcrc);

    printf("\n");

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

static int parse_and_close_png_file(FILE * f, int skipidat, int showpalette, int showpalettecolors, int verifycrc)
{
    struct myruntime runtime;
    int ret;

    memset(&runtime, 0x0, sizeof(struct myruntime));
    runtime.f = f;
    runtime.skipidat = skipidat;
    runtime.showpalette = showpalette;
    runtime.showpalettecolors = showpalettecolors;
    runtime.verifycrc = verifycrc;
    ret = setjmp_and_doit(&runtime);

    /* if it was stdin then don't close it */
    if(f != stdin)
        fclose(f);

    return ret;
}

static int handle_file(const char * fname, int skipidat, int showpalette, int showpalettecolors, int verifycrc)
{
    FILE * f;
    if(samestring(fname, "-"))
        f = stdin;
    else
        f = my_utf8_fopen_rb(fname);

    if(f)
    {
        if(samestring(fname, "-"))
            printf("File '<stdin>'\n");
        else
            printf("File '%s'\n", fname);

        return parse_and_close_png_file(f, skipidat, showpalette, showpalettecolors, verifycrc);
    }

    fprintf(stderr, "Error: fopen('%s') = NULL\n", fname);
    return 1;
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

#define OPTION_STRINGS_COUNT 8
const char * const kAllOptionStrings[OPTION_STRINGS_COUNT] = {
    "--no-idat",
    "--plte", "--color-plte",
    "-h", "--help",
    "--crc",
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
    int i, anyerrs, skipidat, files, showpalette, showpalettecolors, verifycrc;

    applyOpenBsdRestrictions(argc, argv);
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
    verifycrc = count_exact_option_presence(argc, argv, "--crc");
    if((argc - skipidat) < 2)
        return print_usage(argv[0], stderr);

    /* only enable colors if they're needed */
    if(showpalettecolors)
        enableConsoleColor();

    anyerrs = 0;
    files = 0;
    for(i = 1; i < argc; ++i)
    {
        if(isoption(argv[i]))
            continue;

        if(files > 0)
            printf("\n");

        ++files;
        anyerrs += handle_file(argv[i], !!skipidat, !!showpalette, !!showpalettecolors, !!verifycrc);
    } /* for */

    return !!anyerrs;
}
