#!/usr/bin/env python3
"""Static PE checks only. Does not execute Windows code or inspect the game."""

import argparse
from pathlib import Path
import re
import struct
import subprocess


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def inspect(path, objdump, architecture, is_dll=False):
    data = path.read_bytes()
    require(len(data) >= 64 and data[:2] == b'MZ', f'{path.name}: missing DOS header')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    require(pe + 26 <= len(data) and data[pe:pe+4] == b'PE\0\0', f'{path.name}: missing PE header')
    machine, magic, label = (0x8664, 0x20b, 'PE32+ AMD64') if architecture == 'x64' else (0x14c, 0x10b, 'PE32 i386')
    require(struct.unpack_from('<H', data, pe+4)[0] == machine, f'{path.name}: expected {architecture}')
    require(struct.unpack_from('<H', data, pe+24)[0] == magic, f'{path.name}: expected {label}')
    characteristics = struct.unpack_from('<H', data, pe+22)[0]
    require(bool(characteristics & 0x2000) == is_dll, f'{path.name}: wrong DLL characteristic')
    details = subprocess.check_output([objdump, '-p', str(path)], text=True)
    imports = set(re.findall(r'DLL Name:\s*(\S+)', details))
    require(imports and {s.lower() for s in imports} <= {'kernel32.dll', 'msvcrt.dll'},
            f'{path.name}: unexpected imports: {sorted(imports)}')
    if is_dll:
        for name in ('WolfUW_ApiVersion', 'WolfUW_Initialize', 'WolfUW_Shutdown'):
            require(re.search(r'\b' + re.escape(name) + r'\s*$', details, re.M), f'Missing undecorated export: {name}')
        require('The .rsrc Resource Directory section:' in details, 'Missing version resource')
    print(f'PASS: {path.name}: {label} {"DLL" if is_dll else "EXE"}; imports: {", ".join(sorted(imports))}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-dir', type=Path, required=True)
    parser.add_argument('--arch', choices=['x86', 'x64'], default='x86')
    parser.add_argument('--objdump', help='Matching MinGW objdump; defaults to the selected architecture')
    args = parser.parse_args()
    objdump = args.objdump or ('x86_64-w64-mingw32-objdump' if args.arch == 'x64' else 'i686-w64-mingw32-objdump')
    inspect(args.bin_dir / 'WolfUltrawide.dll', objdump, args.arch, is_dll=True)
    inspect(args.bin_dir / 'WolfUWHost.exe', objdump, args.arch)
    inspect(args.bin_dir / 'wolf_core_tests.exe', objdump, args.arch)
    print('PASS: DLL API exports and version resource. Not executed: Windows loading or game behavior.')


if __name__ == '__main__':
    main()
