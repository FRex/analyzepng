# analyzepng

Small stadalone C program to load a single png file and print info about its chunks.

```
$ ./analyzepng32v19_04_01.exe
analyzepng32v19_04_01.exe - print information about chunks of a given png file
Usage: analyzepng32v19_04_01.exe file.png
```

```
$ ./analyzepng32v19_04_01.exe test.png
File 'test.png'
IHDR, 13 bytes, 206 x 131
IDAT, 31814 bytes
IEND, 0 bytes
This PNG has: 3 chunks, 31871 bytes
```

Go to releases to download a Windows exe compiled with Pelles C with no `-O2`
to avoid running into any `-O2` optimizer bug similar to this one that affected
`stb_image`: [Pelles C forum bug report](https://forum.pellesc.de/index.php?topic=7837.0)
