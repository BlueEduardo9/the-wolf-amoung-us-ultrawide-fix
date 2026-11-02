# Building

CMake 3.25+, C++17 and a build tool are required. There are no fetched libraries
or bundled game dependencies. The runtime target is currently x86 only.

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
the MSVC x86 tools, a Windows SDK, and CMake 3.25+. In PowerShell from the source
root:

```powershell
cmake --preset windows-x86
cmake --build --preset windows-x86
ctest --preset windows-x86
& .\scripts\Test-Windows.ps1 -BinDir .\work\build-msvc-x86\Release
```

The preset selects **Win32**, even on 64-bit Windows, and uses the static MSVC
runtime. Native Windows CTest includes a DLL smoke test in addition to the core
test. The PowerShell script adds failure and Unicode-path coverage. The MSVC
recipe has also passed on GitHub's Windows Server 2022 runner; use the validation
record for the exact observed results. Your own Windows PC and Steam game still
need local testing.

If local PowerShell policy blocks a downloaded script, review the script and use
your normal approved method for running local development scripts. This project
does not change machine-wide execution policy.

## CI

The included workflow checks the source allowlist and builds/tests Win32 on a
Windows runner. It packages a standalone artifact with this project's binaries,
example INI, inspection/smoke scripts and instructions. A successful run is linked
in the validation record. These checks do not establish game compatibility.
