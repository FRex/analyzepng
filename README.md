# analyzepng

Small stadalone C program to load png files and print info about their chunks,
and other general information (size, trailing data, compression efficiency).

Tries to be robust, not print trash, handle any chunk length, etc. if chunk ID
is not letters they will be printed as a hex number (see bad4cc.png below).
Should work on any OS and any C compiler.

On Windows it should build with terminal color and UTF-16 filename support,
this can be verified by presence of line `Windows build capableof colors and UTF-16 filenames`
in the help/usage message. The UTF-16 filenames in the command output (`File ''` lines)
they will be printed as UTF-8. On other OSes this line is not present and they are
assumed to be using UTF-8 filenames and have color enabled terminals by default.

Eval result of `+set-bash-completion` to set tab completion in bash. The
options meant for bash scripting start with a single `+` instead of `--` to
not be completed when you type in `--` and trigger completion.

```
$ analyzepng.exe -h
analyzepng.exe - print information about chunks of given png files
Windows build capableof colors and UTF-16 filenames
Usage: analyzepng.exe [--no-idat] file.png...
    --h OR --help #print this help to stdout
    --no-idat #don't print IDAT chunk locations and sizes, can be anywhere
    --plte #print RGB values from the PLTE chunk
    --color-plte #print RGB values from the PLTE chunk using ANSI escape codes
    +set-bash-completion #print command to set bash completion
    +do-bash-completion #do completion based on args from bash
```

```
$ analyzepng.exe test.png
File 'test.png'
IHDR, 13 bytes at 16, 206 x 131, 8-bit RGBA
IDAT, 31814 bytes at 41
IEND, 0 bytes at 31867
This PNG has: 3 chunks (1 IDAT), 31871 bytes (31.124 KiB) and contains 107944 bytes (105.414 KiB) of image data (29.53%)
```

Go to releases to download a Windows exe compiled with Pelles C with no `-O2`
to avoid running into any `-O2` optimizer bug similar to this one that affected
`stb_image`: [Pelles C forum bug report](https://forum.pellesc.de/index.php?topic=7837.0)

Option `--no-idat` skips printing `IDAT` chunks, which can be useful to not clutter
the output if there are very many. Total `IDAT` count is still displayed at the end.
Use `--plte` or `--color-plte` to print the palette from the `PLTE` chunk (if present).

```
$ analyzepng.exe 2000px-Pacman.png
File '2000px-Pacman.png'
IHDR, 13 bytes at 16, 2000 x 2107, 8-bit RGBA
IDAT, 8192 bytes at 41
IDAT, 8192 bytes at 8245
IDAT, 8192 bytes at 16449
IDAT, 8192 bytes at 24653
IDAT, 8192 bytes at 32857
IDAT, 8192 bytes at 41061
IDAT, 8192 bytes at 49265
IDAT, 8087 bytes at 57469
IEND, 0 bytes at 65568
This PNG has: 10 chunks (8 IDAT), 65572 bytes (64.035 KiB) and contains 16856000 bytes (16.075 MiB) of image data (0.39%)

$ analyzepng.exe --no-idat 2000px-Pacman.png
File '2000px-Pacman.png'
IHDR, 13 bytes at 16, 2000 x 2107, 8-bit RGBA
IEND, 0 bytes at 65568
This PNG has: 10 chunks (8 IDAT), 65572 bytes (64.035 KiB) and contains 16856000 bytes (16.075 MiB) of image data (0.39%)
```

```
$ analyzepng.exe trailing.png adam7.png
File 'trailing.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (1 IDAT), 200 bytes (0.195 KiB) and contains 3627 bytes (3.542 KiB) of image data (5.51%)
Error: 6 bytes (0.006 KiB) of trailing data

File 'adam7.png'
IHDR, 13 bytes at 16, 16 x 16, 8-bit RGBA, Adam7 interlaced
bKGD, 6 bytes at 41
pHYs, 9 bytes at 59, 2835 x 2835 pixels per meter
tIME, 7 bytes at 80, 2005-03-10 11:52:57
tEXt, 8 bytes at 99, full (as ASCII + escapes): Comment\0
IDAT, 571 bytes at 119
IEND, 0 bytes at 702
This PNG has: 7 chunks (1 IDAT), 706 bytes (0.689 KiB) and contains 1024 bytes (1.000 KiB) of image data (68.95%)
```

```
$ analyzepng.exe trailing5gig.png
File 'trailing5gig.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (1 IDAT), 200 bytes (0.195 KiB) and contains 3627 bytes (3.542 KiB) of image data (5.51%)
Error: over 15728640 bytes (15.000 MiB) of trailing data
```

```
$ analyzepng bmp.bmp jpg.jpg GIF87a.gif GIF89a.gif wav.wav webp.webp mng.mng analyzepng.exe
File 'bmp.bmp'
Error: starts with 'BM', like a BMP file

File 'jpg.jpg'
Error: starts with 0xff 0xd8 0xff, like a JPEG file

File 'GIF87a.gif'
Error: starts with 'GIF', like a GIF file

File 'GIF89a.gif'
Error: starts with 'GIF', like a GIF file

File 'wav.wav'
Error: starts with 'RIFF', but has no 'WEBP' at offset 8

File 'webp.webp'
Error: starts with 'RIFF' and has 'WEBP' on offset 8, like a WEBP file

File 'mng.mng'
Error: MNG 8-byte header valid, this tool only handles PNG

File 'analyzepng.exe'
Error: PNG 8-byte header has unknown wrong values: 0x4d 0x5a 0x90 0x00 0x03 0x00 0x00 0x00 "MZ......"
```

```
$ analyzepng.exe bad4cc.png
File 'bad4cc.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
0x00004154, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (0 IDAT), 200 bytes (0.195 KiB) and contains 3627 bytes (3.542 KiB) of image data (5.51%)
```

```
$ analyzepng.exe jumpscare.png
File 'jumpscare.png'
IHDR, 13 bytes at 16, 465 x 465, 8-bit RGBA
acTL, 8 bytes at 41, APNG animation control
fcTL, 26 bytes at 61, APNG frame control
IDAT, 98899 bytes at 99
fcTL, 26 bytes at 99010, APNG frame control
fdAT, 236069 bytes at 99048, APNG frame data
IEND, 0 bytes at 335129
This PNG has: 7 chunks (1 IDAT), 335133 bytes (327.278 KiB) and contains 864900 bytes (844.629 KiB) of image data (38.75%)
```
