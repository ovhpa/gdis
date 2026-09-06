## GDIS, Copyright (C) 2000-2015 by Sean Fleming, Andrew Rohl

<andrew.rohl@curtin.edu.au>

GDIS comes with ABSOLUTELY NO WARRANTY.

This is free software. You are welcome to redistribute copies provided the conditions of the Version 2 GPL (GNU Public License) are met.

Although you are not required to do so, the authors would consider it a courtesy if you submit to them any changes you consider to be worthwhile. The goal would be to keep the development of GDIS more
or less centralized.

This version is an effort to port the GDIS project to the QT6 graphical user interface, propelled by [Okadome Valencia](hubert.okadome.valencia@ovhpa.net) with a significant help from the [QWEN 3.6](https://github.com/QwenLM/Qwen3.8) LLM project.
This new interface work is still at its very beginning; feel free to contact us via email, issue, or pull request whenever you will encounter a problem.

### Installation (Qt6 / CMake)

GDIS is now built with **Qt6** and the **CMake** toolchain. To build from source:

```bash
# 1. Install build dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake pkg-config \
  libgl1-mesa-dev libglu1-mesa-dev \
  qtbase6-dev qtdeclarative6-dev qttools6-dev \
  libqt6opengl6-dev libqt6openglwidgets6-dev \
  libglib2.0-dev

# 2. Configure and build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 3. The binary is placed in bin/
./bin/gdis
```

**Optional features:**

- **Grisu grid support**: `cmake .. -DUSE_GRISU=ON` (requires `kdsoap2-qt6`)
- **Cairo/EPS export**: `cmake .. -DUSE_CAIRO=ON` (requires `libcairo2-dev`)

Note: at present these two options have **not** been tested, so it is not recommended to use any.

### Notes

If you are compiling on a Mac, it is recommended that you use [MacPorts](https://www.macports.org) to install Qt6 and Mesa. Homebrew is not yet officially supported (but it is a WIP, see [gdis.rb](gdis.rb)).

### Project files and organisation

The file [LICENSE](LICENSE) descibe the GPLv2 licensing info.

The file [CONTRIB](CONTRIB.md) is an effort to acknowledge the past (and present) contributors to the GDIS project.

The `models` directory gives some examples of the many formats supported by GDIS. Note that not all the formats are represented here.

The `gui` directory contains the `C++` code related to the QT6 interface.

The `src` contains the `C` core code of GDIS, including the original OpenGL render, tools, and features of the original GDIS project.


