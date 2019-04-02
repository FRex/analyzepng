# analyzepng

Small stadalone C program to load a single png file and print info about its chunks.

```
$ analyzepng.exe
analyzepng.exe - print information about chunks of a given png file
Usage: analyzepng.exe file.png
```

```
$ analyzepng.exe test.png
File 'test.png'
IHDR, 13 bytes at 16, 206 x 131
IDAT, 31814 bytes at 41
IEND, 0 bytes at 31867
This PNG has: 3 chunks, 31871 bytes
```

Go to releases to download a Windows exe compiled with Pelles C with no `-O2`
to avoid running into any `-O2` optimizer bug similar to this one that affected
`stb_image`: [Pelles C forum bug report](https://forum.pellesc.de/index.php?topic=7837.0)

```
$ analyzepng.exe trailing.png
File 'trailing.png'
IHDR, 13 bytes at 16, 39 x 31
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks, 200 bytes
Error: 6 bytes of trailing data
```

```
$ analyzepng.exe trailing5gig.png
File 'trailing5gig.png'
IHDR, 13 bytes at 16, 39 x 31
PLTE, 6 bytes at 41
IDAT, 125 bytes at 59
IEND, 0 bytes at 196
This PNG has: 4 chunks, 200 bytes
Error: over 15728768 bytes of trailing data
```
