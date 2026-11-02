"""Validate source-built Akuma's Phase 82A cancel and projectile invariants.

Run after a fresh import from personally owned game files. No game assets
are embedded, logged, uploaded, or copied into the report.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from ttt2char.ttt2 import motbin

JUMPS = {
    'Mx_sJUMP_00_': ('Mx_tatumaki_air', 'Mx_tatumaki_airEX'),
    'Mx_sJUMP_00F': ('Mx_tatumaki_airF', 'Mx_tatumaki_airEXF'),
    'Mx_sJUMP_00B': ('Mx_tatumaki_airB', 'Mx_tatumaki_airEXB'),
    'Mx_sJUMPr00_': ('Mx_tatumaki_air', 'Mx_tatumaki_airEX'),
    'Mx_sJUMPr00F': ('Mx_tatumaki_airF', 'Mx_tatumaki_airEXF'),
    'Mx_sJUMPr00B': ('Mx_tatumaki_airB', 'Mx_tatumaki_airEXB'),
}


def validate(m):
    failures, passed = [], []
    names = {m.move_name(i): i for i in range(len(m.tables['moves']))}
    cancels = m.tables['cancels']
    for jump, (normal, ex) in JUMPS.items():
        label = f'{jump} -> {normal}/{ex}'
        if not all(name in names for name in (jump, normal, ex)):
            failures.append(label + ': missing move')
            continue
        start = m.tables['moves'][names[jump]]['cancel']
        if start is None or start + 6 > len(cancels):
            failures.append(label + ': missing jump cancel list')
            continue
        rows = cancels[start:start + 6]
        expected = [names[ex]] * 2 + [names[normal]] * 4
        moves = [r['move_id'] for r in rows]
        directions = [r['direction'] for r in rows]
        if moves != expected or any(d < 0x800D or d >= 0xC000 for d in directions):
            failures.append(label + ': six source EX/normal routes not found at list head')
        else:
            passed.append(label)
    for move, expected in (('Ry_EX_hadoh', 2), ('Ry_shinhadouken', 5)):
        if move not in names:
            failures.append(move + ': missing move')
            continue
        extra = m.tables['extra_move_properties']
        start = m.tables['moves'][names[move]]['extraprops']
        if start is None:
            failures.append(move + ': missing projectile spawn properties')
            continue
        spawns = []
        for row in extra[start:min(len(extra), start + 128)]:
            if row['id'] == 0:
                break
            if row['id'] == 0x8168:
                spawns.append(row['value'])
        if len(spawns) != 1 or not (0 < spawns[0] < len(m.tables['projectiles'])):
            failures.append(move + ': expected exactly one valid projectile spawn')
            continue
        raw = m.tables['projectiles'][spawns[0]]['raw']
        hits = struct.unpack('>34I', raw)[4]
        if hits != expected:
            failures.append(f'{move}: {hits} projectile hit(s), expected {expected}')
        else:
            passed.append(f'{move}: {hits} native projectile hits')
    return {'passed': passed, 'failures': failures, 'pass_count': len(passed),
            'fail_count': len(failures), 'ok': not failures}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('moveset', type=Path, help='freshly imported mrx.moveset')
    parser.add_argument('--report', type=Path, help='write asset-free JSON verification report')
    args = parser.parse_args()
    source = args.moveset.resolve()
    data = source.read_bytes()
    m = motbin.read(data)
    result = validate(m)
    result.update({'source': str(source), 'sha256': hashlib.sha256(data).hexdigest(),
                   'file_size': len(data)})
    output = json.dumps(result, indent=2)
    print(output)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(output + '\n', encoding='utf-8')
    return 0 if result['ok'] else 2


if __name__ == '__main__':
    sys.exit(main())
