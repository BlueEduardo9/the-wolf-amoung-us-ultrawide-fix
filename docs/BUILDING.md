# Building

CMake 3.25+, C++17 and a build tool are required. There are no fetched libraries
or bundled game dependencies. The runtime supports x86 and x64 Windows targets.
Use the architecture of the executable you inspected; an x86 DLL cannot load
into an x64 game process. No game integration is verified for either target.

## Prepared cloud Windows toolchain

Run from this repository in Bash:

```sh
source /workspace/.cloud-setup/wolf-windows/activate.sh
cmake --preset cloud-win32
cmake --build --preset cloud-win32 --parallel 2
python scripts/verify_pe.py --bin-dir work/build-win32
```

The activation script selects the prepared i686 MinGW-w64 compiler, CMake and
Ninja and sets `WOLF_MINGW32_TOOLCHAIN`. Nothing is installed into the Steam
directory. The environment's toolchain files are external build prerequisites,
not files to add to Git. This preset requires that activation step.

Outputs under `work/build-win32/`:

- `WolfUltrawide.dll`: observe-only Windows PE32/i386 DLL.
- `WolfUWHost.exe`: x86 Windows diagnostic host.
- `wolf_core_tests.exe`: x86 Windows synthetic tests.
- `WolfUltrawide.ini`: example configuration copied beside the DLL.

MinGW's C++/GCC/thread runtimes are statically linked. The PE verifier checks
architecture, DLL characteristics, exports, version resource and allowed Windows
system imports. It **does not execute** these files. Do not run CTest on this
cross-build and interpret failure to execute a PE as a game result. Transfer the
four named files to a writable folder on Windows for runtime testing.

## Portable core tests in this environment

```sh
source /workspace/.cloud-setup/wolf-windows/activate.sh
cmake --preset native-tests
cmake --build --preset native-tests --parallel 2
ctest --preset native-tests
./work/build-native/wolf_core_tests
```

These tests run native Linux code for configuration, math and the synthetic
buffer scanner. They neither load a Windows DLL nor validate game behavior.

## Native Windows build with Visual Studio

Install Visual Studio 2022 or Build Tools with **Desktop development with C++**,
the MSVC x86/x64 tools, a Windows SDK, and CMake 3.25+. For an x64 executable,
run these commands in PowerShell from the source root:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
ctest --preset windows-x64
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Test-Windows.ps1 -BinDir .\work\build-msvc-x64\Release -ExpectedArchitecture x64
```

For an x86 executable, substitute `windows-x86`, `build-msvc-x86` and
`-ExpectedArchitecture x86`. Both presets use the static MSVC runtime and
separate build folders. Native Windows CTest includes a DLL smoke test in
addition to the core test. The PowerShell script checks that the host, DLL and
tests have matching PE architectures, then runs failure and Unicode-path cases.
The validation record distinguishes observed x86 results from later x64 CI
results; compilation alone never establishes game compatibility.

The example's `-ExecutionPolicy Bypass` applies only to the new PowerShell process;
it does not change saved machine or user execution-policy settings.

## CI

The included workflow checks the source allowlist and builds/tests x86 and x64 in
separate Windows jobs. It packages separate standalone artifacts with this project's binaries,
example INI, inspection/smoke scripts and instructions. A successful run is linked
in the validation record. These checks do not establish game compatibility.
