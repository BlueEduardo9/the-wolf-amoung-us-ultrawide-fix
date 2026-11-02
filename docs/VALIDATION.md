# Validation record

Recorded 2026-11-02 using the prepared Linux build environment and GitHub's
Windows Server 2022 runner. No game files were available or used. No Steam
installation or game was executed.

The baseline table below records the original x86 build. x64 support was added
after local read-only inspection identified an x64/PE32+ target. That report is
not a verified game profile. The prepared cloud toolchain remains x86; the new
x64 build, host/DLL tests, inspection script and architecture guards run in a
separate Windows CI job. Check the matching successful job in
[Actions](https://github.com/BlueEduardo9/the-wolf-amoung-us-ultrawide-fix/actions)
for a later artifact; do not infer x64 test results from the x86 baseline.

| Check | Observed result |
| --- | --- |
| Prepared toolchain | MinGW-w64 i686 GCC 14 POSIX; CMake 3.31.6; Ninja 1.11.1 |
| Cross-compile/link | PASS: DLL, test host, Windows core test EXE and version resource |
| PE architecture | PASS: all three outputs are PE32/i386; DLL characteristic is correct |
| DLL exports | PASS: undecorated `WolfUW_ApiVersion`, `WolfUW_Initialize`, `WolfUW_Shutdown` |
| Static import inspection | PASS: each output imports only `KERNEL32.dll` and `msvcrt.dll` |
| Native core tests | PASS: 43 synthetic checks, 0 failures, native GCC 14.2.0 |
| Python syntax | PASS: both Python utility scripts compile |
| Git exclusions | PASS: sampled EXE/DLL/archive/save/screenshot/dump/build/local paths are ignored |
| Source publication guard | PASS: explicit source allowlist; forced extra binary rejected in an isolated fixture |
| Native MSVC build | PASS: Visual Studio 2022, MSVC 19.44, Win32 Release on Windows Server 2022 |
| Windows core EXE | PASS: 43 synthetic checks |
| Windows DLL load/init/unload | PASS: standalone host, outside the game |
| PowerShell scripts on Windows | PASS: smoke/failure cases, Unicode config/log paths, and PE/SHA-256 inspection of our own host executable |
| In-game loading | NOT RUN; no validated loader integration supplied |
| Game patch targets/semantics | NOT DISCOVERED; no game addresses/signatures supplied |
| Steam game visuals at 3440×1440 | NOT RUN; no functional-fix claim |
| GitHub source publication | 26 source/configuration/documentation files published with user approval; game files excluded |
| GitHub Actions artifact | PASS: standalone Windows starter ZIP uploaded; no game content |

Observed Windows results and the downloadable artifact are in
[successful CI run 41525954749](https://github.com/BlueEduardo9/the-wolf-amoung-us-ultrawide-fix/actions/runs/41525954749),
for source commit `015ce057f642214107058193ce391368d98042f4`. The first run
identified a retained PowerShell exit code after expected negative cases; that
script result was corrected and the complete workflow then passed. Unicode
diagnostic paths also print correctly in the captured CI log.

The native tests cover config parsing and failure cases, exact 3440×1440 aspect
and reference geometry, FOV math, and unique/missing/ambiguous synthetic pattern
matches. They do not validate game memory layouts, rendering or input.

The CI smoke script verified `PATCH_STATUS=no_verified_profile`,
`MEMORY_WRITES=0` and `HOOKS_INSTALLED=0` in its standalone Windows host log. It
did not load the DLL into the game. Any in-game compatibility record must be
added only after testing the actual Steam EXE, with its SHA-256/build ID and
tested scenes. Keep local evidence out of Git.
