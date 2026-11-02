"""Source-height Demon Flip regression fixtures; no game data."""
import collections
import copy
import struct
import unittest
from types import SimpleNamespace
from unittest.mock import patch

from test_arcs import made_up
from ttt2char.convert import arcs, moveset, idmaps
from ttt2char.ttt2 import animation


class SourceArcs(unittest.TestCase):
    def test_mode3_physics_mode4_additive_pose(self):
        dec = made_up(12)
        before = dec['values'].copy()
        _, values = arcs.source_motion(dec, 750, f0=5, modes=[(6, 4)])
        import numpy as np
        np.testing.assert_array_equal(values[:5], before[:5])
        self.assertEqual(values[5, 0, 1] + values[5, 1, 1], 750)
        np.testing.assert_allclose(values[6:, 1, 1], 750 + before[6:, 0, 1] + before[6:, 1, 1])
        np.testing.assert_array_equal(dec['values'], before)
        np.testing.assert_array_equal(values[:, 2:], before[:, 2:])
        np.testing.assert_array_equal(values[:, :2, 0], before[:, :2, 0])

    def test_unknown_mode_fails(self):
        with self.assertRaisesRegex(ValueError, 'mode'):
            arcs.source_motion(made_up(12), 750, modes=[(0, 2)])

    def test_floor_samples_combined_height_before_ground_fall(self):
        dec = made_up(12)
        self.assertEqual(arcs.landing_height(dec, 8), float(dec['values'][7, 0, 1] + dec['values'][7, 1, 1]))
        self.assertEqual(struct.unpack('>f', struct.pack('>I', arcs.float_word(750.5)))[0], 750.5)

    def test_pause_schedule_keeps_event_clock(self):
        props = [(0x8001, 0x822B, 1), (0x8008, 0x822B, 0)]
        self.assertEqual([r['frame'] for r in arcs.followup_pause_rows('Mx_hyaki_gouB', props)], [0x8001, 0x8008])
        self.assertEqual(arcs.followup_pause_rows('another_fighter', props), [])
        with self.assertRaisesRegex(ValueError, 'unverified'):
            arcs.followup_pause_rows('Mx_hyaki_gouD', props[:1])

    def test_ground_conversion_keeps_timing_and_clones_shared_props(self):
        c = object.__new__(moveset.Converter)
        c.k = SimpleNamespace(tables={'moves': [dict(anim=0, ground_fall=10, extraprops=0)]})
        c.newidx = {0: 0}
        c.T = {'moves': [dict(anim=99, extraprops=0, ground_fall=10, cancel=42)],
               'extra_move_properties': [dict(frame=0x8001, id=arcs.BAKED, value=0x82466405), dict(frame=0, id=0, value=0)]}
        original = copy.deepcopy(c.T)
        c.new_anims, c.pack0_count, c.stats = [], 0, collections.Counter()
        with patch.object(arcs, 'decode_t7', return_value=made_up(12)), patch.object(idmaps, 'xps', return_value=[(6, arcs.MOVEMENT_MODE, 4)]):
            c.source_physics_arc(0, 5, 0x82466400)
        self.assertEqual(c.T['extra_move_properties'][:2], original['extra_move_properties'])
        self.assertEqual((c.T['moves'][0]['ground_fall'], c.T['moves'][0]['cancel']), (10, 42))
        marker = c.T['extra_move_properties'][3]
        self.assertEqual((marker['frame'], marker['id'], marker['value']), (5, arcs.BAKED, 0x82466485))
        self.assertEqual(animation.decode(c.new_anims[0], '>')['frames'], 12)


if __name__ == '__main__':
    unittest.main()
