# The Wolf Among Us ultrawide research starter

A Windows C++ starter for your own Steam ultrawide work at **3440×1440**.

**Status: observe-only scaffold, not a working game fix.** No game addresses,
signatures, hooks, executable hashes, or compatible Steam builds have been
verified. The game has not been run here. The DLL never changes resolution,
projection, FOV, black bars, HUD, subtitles, or game memory.

The prepared 32-bit MinGW-w64 tools build `WolfUltrawide.dll`, a separate
`WolfUWHost.exe` smoke-test host, and `wolf_core_tests.exe`. This is an **x86**
starter; verify your actual Steam executable's architecture before considering
in-game loading. Windows x64 can run the x86 test host.

## Start here

For a Windows download without building locally, open this repository's **Actions**
tab, select a successful **Build starter (no game files)** run, and download
**observe-only-starter-win32** from its **Artifacts** section. Extract the ZIP
fully into a writable folder outside Steam. Open PowerShell in that folder and
run `./Test-Windows.ps1 -BinDir .`. The artifact includes the test scripts and
`START-HERE.txt`; it does not include or modify the game.

The green **Code → Download ZIP** button downloads source only. Build outputs
are in the Actions artifact, not in the Git repository.

1. [Build with the prepared tools or Visual Studio](docs/BUILDING.md).
2. [Run the Windows smoke tests and record a game baseline](docs/WINDOWS_TESTING.md).
3. [Research the exact game build before adding patches](docs/RESEARCH.md).
4. [Read the validation record and its limits](docs/VALIDATION.md).

The default INI selects `Mode=Observe`, `Width=3440`, `Height=1440`. Any other
mode is rejected. These dimensions are diagnostic inputs; they do not set a
display mode. Missing, misspelled, duplicate and invalid configuration keys fail
closed.

## What is included

| Component | Purpose |
| --- | --- |
| `src/windows/runtime.cpp` | Explicit initialization, UTF-8 file logging, host PE metadata, observe-only result |
| `src/windows/game_integration.cpp` | Empty, clearly marked integration point; no game patch data |
| `src/core.cpp` | Validated configuration, aspect/FOV geometry, bounded unique-pattern search |
| `tools/windows_host.cpp` | Loads the DLL into its own process after `LoadLibraryExW` returns |
| `tests/core_tests.cpp` | Synthetic tests; no game content or memory access |
| `scripts/Inspect-Game.ps1` | Reads a local EXE's PE metadata and SHA-256 without copying it |
| `scripts/Test-Windows.ps1` | Runtime smoke tests, Unicode paths and rejected configurations |
| `.github/workflows/build.yml` | Source-only checks plus Windows build/smoke-test recipe |

3440/1440 is **43:18 ≈ 2.388888889**, rather than an exact 21:9 ratio. A centered
16:9 reference rectangle at that height would be 2560×1440, with 440 pixels per
side. That is a geometry calculation, not a discovered HUD layout. FOV helpers
are also pure math; the game's FOV convention has not been established.

## DLL lifecycle

This is an ordinary DLL, not a proxy DLL, automatic injector, or bundled ASI
plugin. Copying it beside the game or renaming it `.asi` is not a validated
loading procedure. `DllMain` only remembers its module handle.

The test host resolves three undecorated C exports:

- `WolfUW_ApiVersion()` → `1`.
- `WolfUW_Initialize(const wchar_t* config, const wchar_t* log)` → `1` for the
  observe-only session; negative values indicate an error.
- `WolfUW_Shutdown()` → no-op while there are no persistent resources or patches.

Initialization must run synchronously **outside the loader lock**, after
`LoadLibrary` returns. Serialize calls and finish them before unloading. Null
paths select the INI and `.log` beside the DLL; explicit paths must be absolute,
and log filenames must end in `.log`. Log parents must already exist. See
`include/wolfuw/api.hpp` for result values. Host exit code `0` means the diagnostic
session completed; it does not mean an ultrawide fix was applied.

## Keep the game out of GitHub

Keep the Steam installation and save directory **outside this repository**.
Use the ignored `local/` directory for fingerprints, notes, screenshots and crash
dumps; use ignored `work/` for builds. Never copy the game's EXE, DLLs, archives,
assets or saves into source directories or upload them as release assets.

`.gitignore` uses an explicit source-file allowlist: newly added paths are ignored
until you add them deliberately. `scripts/check_public_files.py` independently
rejects any tracked/publishable path outside its allowlist, including files
force-added with `git add -f`. Update both lists when adding your own source.

Before a commit or push:

```sh
python scripts/check_public_files.py
git status --short
git diff --cached --stat
git ls-files
```

The guard checks paths; it cannot determine whether someone pasted proprietary
content into an allowed source file. Review the content too. CI never launches
Steam or fetches game files. Its artifact contains only this project's three
build outputs, example INI, two test/inspection scripts and testing instructions.
