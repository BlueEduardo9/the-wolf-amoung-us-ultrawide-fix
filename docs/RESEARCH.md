# Turning this scaffold into your own fix

The current integration function always returns **no verified profile**. There
are no placeholder numeric addresses to fill in and no hidden fallback patch.
All byte sequences in `tests/core_tests.cpp` are invented **test fixtures**, not
The Wolf Among Us signatures.

## Local build record

Keep a record under ignored `local/`, with fields left blank until observed:

```text
Steam build ID / branch:
EXE SHA-256 and architecture:
Game language / installed episodes:
Windows / GPU / display scaling:
Loader and initialization method actually tested:
Baseline resolution and checkpoint:
Candidate purpose and module/PE section:
Signature derivation and exact match count:
Original value/instructions and semantics:
Data type / units / lifetime:
Replacement derivation:
Threading / hook ABI / restoration plan:
Observed test results and local evidence:
```

Do local analysis on your legitimately installed executable. Do not upload the
executable, extracted archives, assets, save files or memory dumps. If you later
publish a narrowly scoped derived signature, review exactly what is included;
the scaffold deliberately supplies none.

## Identify semantics before writing

Separate display-mode selection, viewport/projection, camera FOV, scripted bars,
HUD anchoring, subtitles and input mapping. They may live in different systems
and change during scene transitions. A recurring floating-point value is not
proof that you found the camera aspect ratio.

Determine whether a candidate FOV is horizontal or vertical, degrees or radians,
an angle or a tangent/projection coefficient, and whether it persists. The
provided `HorizontalFovDegrees(v, a)` implements the ordinary perspective formula
`h = 2 * atan(tan(v / 2) * a)` with degree conversion. It makes no assertion about
this game's camera. Likewise, 16:9 is a comparison reference rather than a known
internal aspect assumption.

## Add a build-specific profile only with evidence

Before extending `src/windows/game_integration.cpp`, implement and review these
requirements together:

1. Match a locally measured EXE SHA-256 and machine type. PE timestamp and image
   size from the diagnostic log are not sufficient identity checks. Unknown or
   changed builds must return without installing hooks or writing memory.
2. Locate targets in a known loaded module and bounded readable PE section.
   Use module-relative locations/verified signatures with ASLR in mind. The
   buffer scanner itself does not enumerate memory, validate page permissions,
   or translate file offsets into loaded addresses.
3. Require exactly one match and verify expected original bytes and surrounding
   instruction/data context. Zero or multiple matches must fail closed. Validate
   instruction boundaries, displacement calculations and pointer chains rather
   than adding guessed offsets to a match.
4. Determine a safe initialization time and thread/lifetime. Do not patch code
   while another thread can execute a partially written instruction. A hook
   library, trampoline, calling convention and displaced-instruction relocation
   need explicit design. None are implemented here.
5. If a writer is added, bound every operation, change only necessary page
   protections, check every API result, restore protections, and flush the
   instruction cache for code changes. Track original bytes and ownership for
   rollback. Restore only when the current bytes still match your own change.
6. Keep observe/dry-run behavior separate from an explicit opt-in apply mode;
   adding a verified profile must not silently enable writes. Implement lifecycle
   handling before making `WolfUW_Shutdown` meaningful for patches.
7. Add meaningful synthetic failure tests for the new guards, then run Windows
   and actual Steam game tests. Update the validation record with the exact build
   and observed limits. Compilation and synthetic tests cannot prove visual
   correctness.

The current starter omits the memory writer and hooking dependency intentionally
because no game target or semantics have been established. Add the smallest
mechanism your measured target requires once that research is complete.
