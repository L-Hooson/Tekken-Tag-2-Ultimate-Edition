"""Asset-free positive and negative tests for the fresh-import Akuma pack validator."""
import copy
import struct
import unittest

from verify_akuma_alpha_pack import JUMPS, validate


class SyntheticPack:
    def __init__(self):
        self.names = []
        self.tables = {'moves': [], 'cancels': [], 'projectiles': [],
                       'extra_move_properties': []}
        def add(name, cancels=None, props=None):
            idx = len(self.names)
            self.names.append(name)
            self.tables['moves'].append({'cancel': cancels, 'extraprops': props})
            return idx

        fields = ('default', 'left', 'right', 'back')
        self.tables['reactions'] = [dict(zip(fields, range(2516, 2520)))]
        self.tables['reactions'] += [{} for _ in range(4)]
        self.tables['reactions'] += [dict(zip(fields, range(2504, 2508)))]
        self.tables['hit_conditions'] = [{'reactions': 0}, {'reactions': 5}]
        jumps = {}
        for jump, (normal, ex) in JUMPS.items():
            jumps[jump] = add(jump)
            if normal not in self.names: add(normal)
            if ex not in self.names: add(ex)
        for jump, (normal, ex) in JUMPS.items():
            first = len(self.tables['cancels'])
            self.tables['moves'][self.names.index(jump)]['cancel'] = first
            for i, name in enumerate([ex, ex, normal, normal, normal, normal]):
                self.tables['cancels'].append(
                    {'direction': 0x800E + i, 'move_id': self.names.index(name)})
            self.tables['cancels'].append({'direction': 0x8000, 'move_id': 0})
        for name, hits in (('Ry_EX_hadoh', 2), ('Ry_shinhadouken', 5)):
            start = len(self.tables['extra_move_properties'])
            idx = len(self.tables['projectiles'])
            data = [0] * 34
            data[4] = hits
            self.tables['projectiles'].append({'raw': struct.pack('>34I', *data)})
            self.tables['extra_move_properties'].extend(
                ({'id': 0x8168, 'value': idx}, {'id': 0, 'value': 0}))
            cancel = None
            if name == 'Ry_shinhadouken':
                cancel = len(self.tables['cancels'])
                self.tables['cancels'].append(dict(direction=0x8000, win_start=133, start_frame=133, win_end=32767))
            add(name, cancels=cancel, props=start)
        # Valid source references must be >0, like actual TTT2 projectile rows.
        self.tables['projectiles'].insert(0, {'raw': bytes(136)})
        for r in self.tables['extra_move_properties']:
            if r['id'] == 0x8168: r['value'] += 1

    def move_name(self, i):
        return self.names[i]


class ValidatorTests(unittest.TestCase):
    def test_complete_fresh_import_passes(self):
        result = validate(SyntheticPack())
        self.assertTrue(result['ok'], result['failures'])
        self.assertEqual(result['pass_count'], 10)

    def test_old_super_recovery_fails(self):
        m = SyntheticPack()
        m.tables['cancels'][-1]['win_start'] = 135
        self.assertFalse(validate(m)['ok'])

    def test_shared_ex_reaction_fails(self):
        m = SyntheticPack()
        m.tables['hit_conditions'].append({'reactions': 5})
        self.assertFalse(validate(m)['ok'])

    def test_missing_aerial_route_fails(self):
        m = SyntheticPack()
        m.tables['cancels'][0]['move_id'] = 0
        result = validate(m)
        self.assertFalse(result['ok'])
        self.assertIn('six source EX/normal routes', result['failures'][0])

    def test_wrong_ex_count_fails(self):
        m = SyntheticPack()
        proj = m.tables['projectiles'][1]
        values = list(struct.unpack('>34I', proj['raw']))
        values[4] = 1
        proj['raw'] = struct.pack('>34I', *values)
        self.assertFalse(validate(m)['ok'])

    def test_wrong_super_count_fails(self):
        m = SyntheticPack()
        proj = m.tables['projectiles'][2]
        values = list(struct.unpack('>34I', proj['raw']))
        values[4] = 4
        proj['raw'] = struct.pack('>34I', *values)
        self.assertFalse(validate(m)['ok'])


if __name__ == '__main__':
    unittest.main()
