<p align="center">
 <img width="104" height="104" alt="replay_icon_1024_clean(1)" src="https://github.com/user-attachments/assets/4da78b59-ed32-4a84-a0b0-6777c8fbe851" />



<h1 align="center">Replay</h1>

<p align="center">
  A lightweight, low-level mouse and keyboard input recording and playback engine for Windows.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.0.0-blue" alt="version">
  <img src="https://img.shields.io/badge/platform-Windows-0078D6" alt="platform">
  <img src="https://img.shields.io/badge/Qt-6-41CD52" alt="Qt6">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="license">
</p>

---

## Overview

**Replay** records raw mouse and keyboard input at a low level and plays it back exactly as it happened — clicks, movement, key presses, and timing included. Built with Qt 6 for a clean, native-feeling UI on Windows.

## Features

- **Record** — capture mouse movement, clicks, and keystrokes with accurate timing
- **Playback** — replay recorded input sequences on demand
- **Save / Load** — export recordings to a file and reload them later
- **Global hotkeys** — `F8` to start recording, `Esc` to stop playback
- **Lightweight** — low-level Win32 hooks, no bloat, minimal footprint

## Screenshot

<img width="559" height="408" alt="ss2" src="https://github.com/user-attachments/assets/d28a80c6-cf49-4da2-b8a1-acd9239c373e" />
<img width="521" height="414" alt="ss1" src="https://github.com/user-attachments/assets/91bba356-654e-473e-a592-24ee78ef7783" />


## Getting Started

### Prerequisites

- Windows 10/11
- [MSYS2](https://www.msys2.org/) (UCRT64 environment)
- Qt 6
- A C++ compiler (MinGW via MSYS2)

### Build

```bash
# From an MSYS2 UCRT64 shell
git clone https://github.com/lituz-de/Replay.git
cd Replay

# configure & build (adjust to your build system, e.g. qmake/CMake)
qmake Replay.pro
mingw32-make
```

### Run

```bash
./Replay.exe
```

## Usage

1. Press **Record (F8)** or click the button to start capturing input.
2. Do the actions you want to record.
3. Press **Stop** to end the recording.
4. Press **Play (Esc to stop)** to replay it, or **Save…** to store it for later.
5. Use **Load…** to bring back a previously saved recording.

## License

Replay is licensed under the [MIT License](LICENSE).

Built with **Qt 6**, licensed under [GNU LGPL v3](https://www.gnu.org/licenses/lgpl-3.0.html).

## Author

Made by [LiTuz](https://github.com/lituz-de)
