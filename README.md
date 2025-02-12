# EPCT (Earphone Channel Test)

EPCT is a lightweight Windows utility that helps users identify the correct orientation of their headphones (Left/Right) through a stereo sound test when connecting them to the computer. Perfect for headphones with worn-out L/R markings or those without any markings at all.

## Features

- Detects headphone connection event
- Plays a stereo test sound (moving from left to right channel)
- Minimal resource usage (<1.5 MB RAM)
- Works in background without UI

## Why Use It?

- Your headphones have worn-out L/R markings
- Your headphones came without L/R markings
- You need a simple tool for testing stereo channels

## How It Works

When you connect your headphones, the program plays a short musical sequence that moves from the left to the right channel. This helps you determine if your headphones are worn correctly:
- If the sound moves from left to right naturally - your headphones are oriented correctly
- If the sound moves from right to left - swap the headphones

## Auto-start with Windows
To make the program start automatically with Windows:

Press Win + R
Type `shell:startup`
Copy EPCT.exe in the opened folder

## Installation

1. Download the latest release from [Releases](../../releases)
2. Run `EPCT.exe`
3. Program will create necessary files in temp directory on first run

## Building from Source

Requirements:
- MinGW-w64 or other C++ compiler with Windows API support
- Windows SDK

### Compile resources
`windres resource.rc -O coff -o resource.res`

### Compile program
`g++ -O2 -o EPCT.exe epct.cpp resource.res -lwinmm -mwindows -DUNICODE`

## License

[CC BY-NC 4.0](LICENSE)

This software is licensed under Creative Commons Attribution-NonCommercial 4.0 International License. This means you can freely use and modify the code for non-commercial purposes, as long as you provide attribution to the original author.

Commercial use is prohibited without explicit permission from the author.

## Author
ran4erep

Last updated: 2025-02-11
