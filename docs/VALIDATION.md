# Validation record

Recorded 2026-11-02 in the prepared Linux build environment. No game files were
available or used. No Windows runtime, Steam installation or game was executed.

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
| Native MSVC build | NOT RUN; commands and CI recipe supplied |
| Windows core EXE | NOT RUN; cross-compilation is not execution |
| Windows DLL load/init/unload | NOT RUN; smoke-test script supplied |
| PowerShell scripts on Windows | NOT RUN; source reviewed only |
| In-game loading | NOT RUN; no validated loader integration supplied |
| Game patch targets/semantics | NOT DISCOVERED; no game addresses/signatures supplied |
| Steam game visuals at 3440×1440 | NOT RUN; no functional-fix claim |
| GitHub Actions / push / publication | NOT RUN; workflow source supplied, nothing pushed |

The native tests cover config parsing and failure cases, exact 3440×1440 aspect
and reference geometry, FOV math, and unique/missing/ambiguous synthetic pattern
matches. They do not validate game memory layouts, rendering or input.

`PATCH_STATUS=no_verified_profile`, `MEMORY_WRITES=0` and `HOOKS_INSTALLED=0`
are expected runtime fields defined in source, not a captured Windows log. Any
in-game compatibility record must be added only after testing the actual Steam
EXE, with its SHA-256/build ID and tested scenes. Keep local evidence out of Git.
