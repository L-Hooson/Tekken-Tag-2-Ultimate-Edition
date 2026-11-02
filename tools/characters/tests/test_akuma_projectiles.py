"""Asset-free regression tests for Akuma's two-hit EX projectile conversion."""
import struct
import unittest
from tools.characters.ttt2char.convert import akuma_projectiles


def row(hits=1, speed=100, hc=10, nxt=None):
    fields = [0] * 42
    fields[9], fields[12] = hits, speed
    return {'raw': struct.pack('<42I', *fields), 'hit_condition': hc,
            'cancel': nxt}


class T7Source:
    game = 't7'

    def __init__(self):
        self.tables = {
            'moves': [{'extraprops': 0}],
            'extra_move_properties': [
                {'frame': 18, 'id': 0x820b, 'value': 1},
                {'frame': 0, 'id': 0, 'value': 0}],
            'projectiles': [row(), row(hc=449, nxt=0), row(hc=451)],
            'cancels': [{'direction': 1, 'move_id': 2, 'option': 0x41},
                        {'direction': 0x8000, 'move_id': 0, 'option': 0x50}],
        }

    def move_name(self, index):
        return 'Ry_EX_hadoh' if index == 0 else 'unexpected'

    def iter_list(self, table, start, predicate):
        items = self.tables[table]
        for i in range(start, len(items)):
            yield i, items[i]
            if predicate(items[i]):
                break


class TestAkumaEXProjectile(unittest.TestCase):
    def test_source_chain(self):
        source = T7Source()
        self.assertEqual(akuma_projectiles.ex_hadoken_hits(source, 1), 2)
        self.assertIsNone(akuma_projectiles.ex_hadoken_hits(source, 2))

    def test_missing_stage_fail_closed(self):
        source = T7Source()
        source.tables['cancels'][0]['move_id'] = 0
        with self.assertRaises(ValueError):
            akuma_projectiles.ex_hadoken_hits(source, 1)

    def test_wrong_speed_fail_closed(self):
        source = T7Source()
        source.tables['projectiles'][2] = row(speed=70, hc=451)
        with self.assertRaises(ValueError):
            akuma_projectiles.ex_hadoken_hits(source, 1)

    def test_unknown_spawn_refuses(self):
        source = T7Source()
        source.tables['extra_move_properties'][0]['value'] = 12345
        with self.assertRaises(ValueError):
            akuma_projectiles.ex_hadoken_hits(source, 12345)


if __name__ == '__main__':
    unittest.main()
