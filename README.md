# analyzepng

Small stadalone C program to load png files and print info about their chunks.

Tries to be robust, not print trash, handle any chunk length, etc. if chunk ID
is not letters they will be printed as a hex number (see bad4cc.png below).
Should work on any OS and any C compiler.

```
$ analyzepng.exe
analyzepng.exe - print information about chunks of given png files
Usage: analyzepng.exe file.png...
```

```
$ analyzepng.exe test.png
File 'test.png'
IHDR, 13 bytes at 16, 206 x 131, 8-bit RGBA
IDAT, 31814 bytes at 41
IEND, 0 bytes at 31867
This PNG has: 3 chunks (1 IDAT), 31871 bytes (31.124 KiB)
```

Go to releases to download a Windows exe compiled with Pelles C with no `-O2`
to avoid running into any `-O2` optimizer bug similar to this one that affected
`stb_image`: [Pelles C forum bug report](https://forum.pellesc.de/index.php?topic=7837.0)

```
$ analyzepng.exe trailing.png adam7.png
File 'trailing.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (1 IDAT), 200 bytes (0.195 KiB)
Error: 6 bytes (0.006 KiB) of trailing data

File 'adam7.png'
IHDR, 13 bytes at 16, 16 x 16, 8-bit RGBA, Adam7 interlaced
bKGD, 6 bytes at 41
pHYs, 9 bytes at 59
tIME, 7 bytes at 80
tEXt, 8 bytes at 99
IDAT, 571 bytes at 119
IEND, 0 bytes at 702
This PNG has: 7 chunks (1 IDAT), 706 bytes (0.689 KiB)
```

```
$ analyzepng.exe trailing5gig.png
File 'trailing5gig.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (1 IDAT), 200 bytes (0.195 KiB)
Error: over 15728640 bytes (15.000 MiB) of trailing data
```

```
$ analyzepng.exe mng.mng
File 'mng.mng'
Error: MNG 8-byte header valid, this tool only handles PNG
```

```
$ analyzepng.exe hehe8gigs.png
File 'hehe8gigs.png'
IHDR, 13 bytes at 16, 1920 x 1080, 8-bit RGB
hehe, 4294967295 bytes at 41
hehe, 4294967295 bytes at 4294967348
IDAT, 3813740 bytes at 8589934655
IEND, 0 bytes at 8593748407
This PNG has: 5 chunks (1 IDAT), 8593748411 bytes (8.004 GiB)
```

```
$ analyzepng.exe bad4cc.png
File 'bad4cc.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
0x00004154, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks (0 IDAT), 200 bytes (0.195 KiB)
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
This PNG has: 7 chunks (1 IDAT), 335133 bytes (327.278 KiB)
```
