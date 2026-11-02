#!/usr/bin/env python3
"""Default-deny publication guard. Does not inspect any game directory."""

import argparse
from pathlib import Path
import subprocess
import sys

PUBLIC_FILES = frozenset({
    '.gitattributes', '.gitignore', 'CMakeLists.txt', 'CMakePresets.json', 'README.md',
    '.github/workflows/build.yml',
    'config/WolfUltrawide.ini',
    'docs/BUILDING.md', 'docs/WINDOWS_TESTING.md', 'docs/RESEARCH.md', 'docs/VALIDATION.md',
    'include/wolfuw/api.hpp', 'include/wolfuw/core.hpp',
    'src/core.cpp', 'src/windows/dllmain.cpp', 'src/windows/exports.def',
    'src/windows/game_integration.cpp', 'src/windows/runtime.cpp',
    'src/windows/runtime.hpp', 'src/windows/version.rc',
    'tools/windows_host.cpp', 'tests/core_tests.cpp',
    'scripts/Inspect-Game.ps1', 'scripts/Test-Windows.ps1',
    'scripts/check_public_files.py', 'scripts/verify_pe.py',
})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--list', action='store_true', help='Print the exact source packaging allowlist')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    if args.list:
        print('\n'.join(sorted(PUBLIC_FILES)))
        return 0
    result = subprocess.run(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
        cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode != 0:
        print('Run this check inside a Git checkout (git init first for a source ZIP).', file=sys.stderr)
        return 1
    paths = {p.decode('utf-8', errors='surrogateescape') for p in result.stdout.split(b'\0') if p}
    unexpected = paths - PUBLIC_FILES
    missing = {p for p in PUBLIC_FILES if not (root / p).is_file()}
    links = {p for p in PUBLIC_FILES if (root / p).is_symlink()}
    if unexpected or missing or links:
        for title, values in [('Unapproved publishable path', unexpected), ('Missing source file', missing), ('Symlink forbidden in source package', links)]:
            for path in sorted(values):
                print(f'{title}: {path}', file=sys.stderr)
        print('Keep game files, builds, fingerprints, saves, screenshots and dumps outside Git.', file=sys.stderr)
        return 1
    print(f'PASS: {len(paths)} publishable files; only explicit starter source/config/docs paths.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
