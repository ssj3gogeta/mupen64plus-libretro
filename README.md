mupen64plus-libretro
====================

Based on recent version of mupen64plus hg (~2.0 rc2) and patches I originally made for mednafen-ps3.
Uses video plugins pulled from https://github.com/paulscode/mupen64plus-ae

To enable a dynarec CPU core you must pass the WITH_DYNAREC value to make:
* make WITH_DYNAREC=x86
* make WITH_DYNAREC=x86_64
* make WITH_DYNAREC=arm

TODO:
* Make sure all filesystem access targets directories a libretro core would be expected to access
* Make core options for important settings (a depth bias setting is badly needed)
