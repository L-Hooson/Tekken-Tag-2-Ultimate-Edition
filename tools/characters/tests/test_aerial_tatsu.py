"""Regression tests for Akuma's source-derived airborne Tatsumaki inputs.

Uses synthetic moveset rows only: no game dumps, assets or binaries.
Run: python -m unittest discover -s tools/characters/tests -p test_aerial_tatsu.py -v
"""
import collections
import copy
import unittest
from types import SimpleNamespace

from tools.characters.ttt2char.convert import aerial_tatsu


class T7Source:
    game = 't7'

    def __init__(self):
        self.names = []
        self.tables = {'moves': [], 'cancels': []}
        self.targets = {name: 100 + n for n, name in enumerate(sorted({
            target for _, _, normal, ex in aerial_tatsu.JUMP_ROUTES
            for target in (normal, ex)
        }))}
        for _, source, normal, ex in aerial_tatsu.JUMP_ROUTES:
            if source in self.names:
                continue
            self.names.append(source)
            start = len(self.tables['cancels'])
            self.tables['moves'].append({'cancel': start})
            for direction, target in (
                (0x811A, ex), (0x811B, ex),
                (0x8115, normal), (0x8116, normal),
                (0x8117, normal), (0x8118, normal),
            ):
                self.tables['cancels'].append(dict(
                    direction=direction, move_id=self.targets[target],
                    req=2349 if target == ex else 2353, extradata=0,
                    win_start=6, win_end=37, start_frame=6, button=3, option=0))
            self.tables['cancels'].append(dict(direction=0x8000, move_id=0))

    def move_name(self, index):
        if index >= 100:
            return next(name for name, i in self.targets.items() if index == i)
        return self.names[index]

    def iter_list(self, table, start, predicate):
        for index in range(start, len(self.tables[table])):
            row = self.tables[table][index]
            yield index, row
            if predicate(row):
                break


class Converter:
    game = 't7'

    def __init__(self, enabled=True):
        self.k = T7Source()
        self.defn = SimpleNamespace(MOVESET={'aerial_tatsu_jumps': enabled})
        self.stats = collections.Counter()
        names = list(dict.fromkeys(
            name for jump, _, normal, ex in aerial_tatsu.JUMP_ROUTES
            for name in (jump, normal, ex)
        ))
        self.T = {'moves': [], 'cancels': []}
        for name in names:
            start = None
            if name.startswith('Mx_sJUMP'):
                start = len(self.T['cancels'])
                self.T['cancels'].extend((
                    dict(direction=1, move_id=0),
                    dict(direction=0x8000, move_id=0),
                ))
            self.T['moves'].append({'name': name, 'cancel': start})

    def move_name(self, index):
        return self.T['moves'][index]['name']

    def conv_reqs(self, index):
        return index + 100, True

    def conv_seq(self, index):
        return {0x811A: 1, 0x811B: 2, 0x8115: 3,
                0x8116: 4, 0x8117: 5, 0x8118: 6}[index + 0x800D]

    def extradata(self, index):
        return index


class TestAerialTatsu(unittest.TestCase):
    def test_six_states_and_36_routes(self):
        converter = Converter()
        self.assertEqual(aerial_tatsu.apply(converter), 36)
        self.assertEqual(converter.stats['Akuma airborne Tatsu cancels restored'], 36)
        for jump, _, normal, ex in aerial_tatsu.JUMP_ROUTES:
            index = next(i for i, m in enumerate(converter.T['moves']) if m['name'] == jump)
            first = converter.T['moves'][index]['cancel']
            routes = converter.T['cancels'][first:first+6]
            self.assertEqual([converter.move_name(r['move_id']) for r in routes],
                             [ex, ex, normal, normal, normal, normal])
            self.assertEqual([r['direction'] for r in routes],
                             [0x800E, 0x800F, 0x8010, 0x8011, 0x8012, 0x8013])
            self.assertEqual(converter.T['cancels'][first+7]['direction'], 0x8000)

    def test_idempotent(self):
        converter = Converter()
        aerial_tatsu.apply(converter)
        before = copy.deepcopy(converter.T)
        self.assertEqual(aerial_tatsu.apply(converter), 0)
        self.assertEqual(converter.T, before)

    def test_opt_out(self):
        converter = Converter(enabled=False)
        before = copy.deepcopy(converter.T)
        self.assertEqual(aerial_tatsu.apply(converter), 0)
        self.assertEqual(converter.T, before)

    def test_missing_move_refuses_patch(self):
        converter = Converter()
        converter.T['moves'].pop()
        with self.assertRaisesRegex(ValueError, 'missing'):
            aerial_tatsu.apply(converter)


if __name__ == '__main__':
    unittest.main()
