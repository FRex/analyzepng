# analyzepng

Small stadalone C program to load a single png file and print info about its chunks.

Tries to be robust, not print trash, handle any chunk length, etc. if chunk ID
is not letters they will be printed as a hex number (see bad4cc.png below).

```
$ analyzepng.exe
analyzepng.exe - print information about chunks of a given png file
Usage: analyzepng.exe file.png
```

```
$ analyzepng.exe test.png
File 'test.png'
IHDR, 13 bytes at 16, 206 x 131, 8-bit RGBA
IDAT, 31814 bytes at 41
IEND, 0 bytes at 31867
This PNG has: 3 chunks, 31871 bytes (31.124 KiB)
```

Go to releases to download a Windows exe compiled with Pelles C with no `-O2`
to avoid running into any `-O2` optimizer bug similar to this one that affected
`stb_image`: [Pelles C forum bug report](https://forum.pellesc.de/index.php?topic=7837.0)

```
$ analyzepng.exe trailing.png
File 'trailing.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks, 200 bytes (0.195 KiB)
Error: 6 bytes (0.006 KiB) of trailing data
```

```
$ analyzepng.exe trailing5gig.png
File 'trailing5gig.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks, 200 bytes (0.195 KiB)
Error: over 15728640 bytes (15.000 MiB) of trailing data
```

```
$ analyzepng.exe hehe8gigs.png
File 'hehe8gigs.png'
IHDR, 13 bytes at 16, 1920 x 1080, 8-bit RGB
hehe, 4294967295 bytes at 41
hehe, 4294967295 bytes at 4294967348
IDAT, 3813740 bytes at 8589934655
IEND, 0 bytes at 8593748407
This PNG has: 5 chunks, 8593748411 bytes (8.004 GiB)
```

```
$ analyzepng.exe bad4cc.png
File 'bad4cc.png'
IHDR, 13 bytes at 16, 39 x 31, 1-bit paletted
PLTE, 6 bytes at 41
0x00004154, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks, 200 bytes (0.195 KiB)
```

```
$ analyzepng.exe Animated_PNG_example_bouncing_beach_ball.png
File 'Animated_PNG_example_bouncing_beach_ball.png'
IHDR, 13 bytes at 16, 100 x 100, 8-bit RGBA
acTL, 8 bytes at 41
fcTL, 26 bytes at 61
IDAT, 4625 bytes at 99
fcTL, 26 bytes at 4736
fdAT, 4215 bytes at 4774
fcTL, 26 bytes at 9001
fdAT, 3922 bytes at 9039
fcTL, 26 bytes at 12973
fdAT, 3713 bytes at 13011
fcTL, 26 bytes at 16736
fdAT, 3409 bytes at 16774
fcTL, 26 bytes at 20195
fdAT, 3058 bytes at 20233
fcTL, 26 bytes at 23303
fdAT, 2672 bytes at 23341
fcTL, 26 bytes at 26025
fdAT, 2493 bytes at 26063
fcTL, 26 bytes at 28568
fdAT, 2636 bytes at 28606
fcTL, 26 bytes at 31254
fdAT, 2670 bytes at 31292
fcTL, 26 bytes at 33974
fdAT, 2585 bytes at 34012
fcTL, 26 bytes at 36609
fdAT, 2487 bytes at 36647
fcTL, 26 bytes at 39146
fdAT, 2555 bytes at 39184
fcTL, 26 bytes at 41751
fdAT, 2662 bytes at 41789
fcTL, 26 bytes at 44463
fdAT, 2718 bytes at 44501
fcTL, 26 bytes at 47231
fdAT, 2765 bytes at 47269
fcTL, 26 bytes at 50046
fdAT, 2865 bytes at 50084
fcTL, 26 bytes at 52961
fdAT, 3076 bytes at 52999
fcTL, 26 bytes at 56087
fdAT, 3372 bytes at 56125
fcTL, 26 bytes at 59509
fdAT, 3872 bytes at 59547
IEND, 0 bytes at 63431
This PNG has: 43 chunks, 63435 bytes (61.948 KiB)
```
