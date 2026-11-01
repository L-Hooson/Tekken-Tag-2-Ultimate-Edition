"""T7 0x8292/0x8293 must remain opt-in until the Akuma runtime adapter exists.

No Tekken assets or generated character packs are used.
"""
import collections
import os
import sys
import types
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from ttt2char.convert import moveset


class ContactActionConversion(unittest.TestCase):
    @staticmethod
    def converter(enabled):
        c = object.__new__(moveset.Converter)
        c.game = 't7'
        c.end = 881
        c.defn = types.SimpleNamespace(MOVESET={}, HITSTOP_CONTACT_ACTIONS=enabled)
        c.cache = {}
        c.true_reqs = set()
        c.stats = collections.Counter()
        c.k_id = 32
        c.newid = 50
        c.gauge = None
        c.map_req = lambda _condition: None  # unsupported action when not opted in
        c.T = {'requirements': []}
        source = [
            dict(condition=0x8292, param=0x00061400),
            dict(condition=0x8293, param=0x01051400),
        ]
        c.req_rows = lambda _index: [(r['condition'], r['param'], r) for r in source]
        return c

    def test_explicit_opt_in_keeps_raw_t7_timers(self):
        c = self.converter(True)
        start, ok = c.conv_reqs(123)
        self.assertTrue(ok)
        self.assertEqual(start, 0)
        self.assertEqual(c.T['requirements'], [
            {'condition': 0x8292, 'param': 0x00061400},
            {'condition': 0x8293, 'param': 0x01051400},
            {'condition': 690, 'param': 0},
        ])
        self.assertEqual(c.stats['contact-stop actions retained'], 2)

    def test_default_does_not_inject_unsupported_ids(self):
        c = self.converter(False)
        start, ok = c.conv_reqs(123)
        if ok:
            self.assertFalse(any(r['condition'] in (0x8292, 0x8293)
                                 for r in c.T['requirements'][start:]))
        self.assertEqual(c.stats['contact-stop actions retained'], 0)


if __name__ == '__main__':
    unittest.main()
