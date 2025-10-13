# 3.1 Requirements & Installation

## Requirements
- Python >= 3.10
- CMake >= 3.26
- GCC >= 13 or MSVC >= 14.10 (Clang: untested)
- C++23 standard or newer

## Installation
### GUI automatic setup (WINDOWS)
Want an easy setup? You can now use a GUI tool created by Mikael. See  [🛠️ Setup Tool (GUI)](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/tool_for_setup.md)

If you use the GUI tool you can skip this and most of the next chapters too, as it automatically covers most of the points. However, if you want detailed knowledge on how everything works (so multiple servers across different VPS, linking your website to the servers, etc), you can keep reading.

### Manual setup (WINDOWS)
1) Clone this repository then go to its folder: `cd <YourEmulatorProjectPath>` - make sure you are inside the MicrovoltsEmulator folder (root of this repository)
2) Clone vcpkg inside ExternalLibraries: `git clone https://github.com/microsoft/vcpkg.git ExternalLibraries\vcpkg`
3) Bootstrap it: `.\ExternalLibraries\vcpkg\bootstrap-vcpkg.bat`
4) Generate build files: `cmake -B build -S . -A x64 -DCMAKE_TOOLCHAIN_FILE=ExternalLibraries/vcpkg/scripts/buildsystems/vcpkg.cmake`
5) Build the project: `cmake --build build --config Release`
6) Output (exes) inside Release folder

### Manual setup (LINUX)
1) Clone this repository then go to its folder: `cd <YourEmulatorProjectPath` - make sure you are inside the MicrovoltsEmulator folder (root of this repository)
2) Clone vcpkg inside ExternalLibraries: `git clone https://github.com/microsoft/vcpkg.git`
3) Bootstrap it: `./ExternalLibraries/vcpkg/bootstrap-vcpkg.sh`
4) Generate build files: `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=ExternalLibraries/vcpkg/scripts/buildsystems/vcpkg.cmake`
5) Build the project: `cmake --build build --config Release`
6) Move the generated elf files in an output folder: `mkdir -p Output` and next `mv AuthServer.elf MainServer.elf CastServer.elf Output/`

## Next
[3.2 Setting up the emulator](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/setting_up.md)



