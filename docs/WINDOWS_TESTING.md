# Windows testing: Steam, 3440×1440

The steps below are instructions to execute locally. They have not been run
against Windows or The Wolf Among Us in this environment. Record each observed
result separately; leave unrun tests marked **not run**.

## 1. Test the DLL without the game

Use Windows 10/11 and a writable folder outside Steam. Build with MSVC or place
the four cross-built outputs together in a folder. From the source repository:

```powershell
# For the prepared cross-build files transferred to this folder:
& .\scripts\Test-Windows.ps1 -BinDir .\work\build-win32
# For a native VS build, use .\work\build-msvc-x86\Release instead.
```

The script runs the Windows core executable, loads/unloads the DLL using the
host, checks log fields, exercises config/log paths containing spaces and a
non-ASCII character, rejects `Mode=Apply`, rejects a missing INI, and checks that
using the config as a log cannot change it. Temporary test files are removed on
exit. The separate DLL/EXE folder should also be tested from a path containing
spaces and non-ASCII characters if that is how you will use it.

For a persistent log, run the host directly from that output folder:

```powershell
.\WolfUWHost.exe --dll .\WolfUltrawide.dll --config .\WolfUltrawide.ini --log .\manual-observe.log
$LASTEXITCODE
Get-Content .\manual-observe.log
```

Expected host exit code: `0`. Expected initialization result: `1`. Expected fields:

```text
HOST_EXE=...WolfUWHost.exe
HOST_MACHINE=0x14c
TARGET_WIDTH=3440
TARGET_HEIGHT=1440
TARGET_ASPECT=2.388888889
PATCH_STATUS=no_verified_profile
MEMORY_WRITES=0
HOOKS_INSTALLED=0
```

These are expected outputs, not captured Windows results. The log must identify
the test host. Host exit codes: `1` for arguments/exports/API exceptions, `2` for
load failure, `3` for a rejected/failed initialization. DLL results are in
`include/wolfuw/api.hpp`. Windows error `193` often means the wrong machine type;
`126` often means a missing module/dependency. Inspect the actual error before
drawing a conclusion.

## 2. Record your actual Steam build

1. In Steam, use the game's **Manage → Browse local files** action. Identify the
   executable Steam actually launches; do not assume a filename from another
   release. Keep that installation outside this repository.
2. Close the game. From this repository, inspect that EXE read-only:

   ```powershell
   $gameExe = Read-Host 'Full path to the executable Steam launches'
   & .\scripts\Inspect-Game.ps1 -Exe $gameExe -OutFile .\local\steam-build.json
   ```

3. Keep the JSON local. Confirm `machine=0x014C`, `architecture=x86`, and
   `optional_header_magic=0x010B` before trying this x86 DLL. If different, stop
   the loading experiment and plan the matching architecture; changing only the
   DLL filename is insufficient.
4. Record Steam's installed build ID, branch, game language, Windows version,
   GPU/driver, display scaling, monitor mode and any other mods/overlays in your
   local notes. The script leaves the Steam build ID blank; it does not query
   Steam. A hash identifies bytes, not compatibility with an existing fix.
5. Back up your saves and existing mod configuration locally. Record a clean
   launch and baseline before introducing a loader or code change.

The file fingerprint and runtime PE metadata have different purposes. This
starter does not compute or check an EXE hash during DLL initialization because
there are no verified profiles or patches to authorize.

## 3. Capture a clean visual baseline

Set the monitor to 3440×1440 and launch through Steam. Record what display modes
the game actually offers; the INI does not add a missing resolution. Capture the
same reproducible camera/scene at an available 16:9 mode and at 3440×1440 if the
game accepts it. If it cannot use 3440×1440, record that as unresolved and first
research display-mode handling.

Use `local/` for all screenshots, videos and notes. Do not distribute game
captures with the source starter. For comparisons, hold the checkpoint, camera,
window/fullscreen mode and display scaling constant. Distinguish side bars,
vertical cropping, stretching and extra horizontal view by observation; do not
assume all black bars have the same cause.

## 4. Establish in-game loading separately

**No in-game loader or loading method is supplied or validated.**
`WolfUWHost.exe` never launches or injects into the game. There is no
`InitializeASI` export or proxy-DLL forwarding layer. Copying or renaming the DLL
alone does not prove it loaded.

If you later choose a compatible loader/debugging method, first test this exact
observe-only DLL and record the loader version, architecture, setup and removal
steps. It must load the DLL and call `WolfUW_Initialize` after loading completes,
outside the loader lock, with absolute writable config/log paths (or the null
defaults beside the DLL). A log naming the actual game host is the first proof
of initialization. A successful standalone host test is insufficient. Do not
describe an unexecuted loading recipe as tested.

No visual changes are expected from the current scaffold. If adding a loader
alone changes behavior, isolate that change before developing a patch.

## 5. After implementing a verified game patch

Use the research checklist first. Recheck the exact EXE hash after Steam updates;
unknown builds must leave the game unchanged. Begin with one narrowly scoped
change and compare the same scenes with the feature disabled and enabled.

| Area | What to record at 3440×1440 |
| --- | --- |
| Launch/display | Steam launch, accepted resolution, fullscreen/windowed mode, alt-tab, focus return |
| Gameplay camera | Aspect, stretching, vertical crop, added horizontal view, camera transitions and zoom |
| Scripted cutscenes | Forced cameras, framing, fades, intentional cinematic bars, occlusion/art at screen edges |
| HUD/subtitles | Positions, text wrapping, readability, clipping and anchoring |
| Menus/dialogue/QTE | Mouse hitboxes, cursor mapping, selection markers, prompts, timers and controller navigation |
| Videos | Prerendered video aspect, stretching and transition to rendered scenes |
| Episodes/saves | Reproducible scenes from each installed episode, reload/checkpoint transitions |
| Lifecycle | Multiple launches, clean exit, reload of settings, crashes, other overlays/mods |
| Guard failures | Unknown build, absent/duplicate signatures, unexpected original bytes, invalid config; zero writes |
| Restore | Disable the patch, remove your own loader/plugin files, launch cleanly, confirm baseline restored |

For each row, record expected behavior, actual behavior, evidence paths and
pass/fail/not-run. A subset passing supports only that subset and exact build.
FOV, bars, HUD and input may require separate solutions. Do not use one successful
scene as proof that all episodes work.

## 6. Roll back

Close the game before removing files. Remove only files you added, using your
recorded loader manifest; restore any local backups you deliberately changed.
This starter patches no files and needs no binary repair. If future development
alters installed files, restore those backups or use Steam's integrity check.
Steam verification may not remove extra third-party loader files, so remove your
own added files explicitly. Launch cleanly and compare with the original baseline.
