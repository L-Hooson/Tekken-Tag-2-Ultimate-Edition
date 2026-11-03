"""Keep the recovered local character choices together across rebuilds."""
import importlib
import collections
import copy
import types
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import unittest
from ttt2char.characters import akuma, eliza, geese, fahkumram, kunimitsu2, noctis
from ttt2char.convert.move_overrides import trim_recovery

class LocalCustomizations(unittest.TestCase):
    def test_all_preset_definitions_survive_consolidation(self):
        expected = {'akuma':7,'chloe':10,'claudio':9,'eliza':8,'fahkumram':7,
                    'geese':7,'gigas':9,'josie':10,'katarina':10,'kazumi':9,
                    'kunimitsu2':9,'leroy':7,'lidia':7,'master_raven':7,
                    'negan':4,'noctis':8,'shaheen':9}
        for name, count in expected.items():
            with self.subTest(fighter=name):
                self.assertEqual(len(importlib.import_module('ttt2char.characters.'+name).PRESETS),count)
    def test_meter_routes_remain_in_all_three_2d_fighters(self):
        # they pay for them from their gauges (convert/gauge.py); Geese's EX moves need his MAX mode
        for c in (akuma,eliza,geese):
            self.assertFalse(c.MOVESET.get('free_meter_specials'),c.NAME)
            self.assertFalse(c.MOVESET.get('free_max_mode'),c.NAME)
            self.assertEqual((c.GAUGE['max'],c.GAUGE['stock']),(1500 if c is geese else 1000,500))
        targets=set(akuma.MOVESET['special_cancels']['Mx_rp00'])
        self.assertTrue({'Ry_EX_hadoh','Ry_EXtatumaki','Ry_shoryuBEX','Ry_shinhadouken'}<=targets)
        self.assertEqual(akuma.MOVESET['special_cancels']['Ry_shoryu'],('Mx_saving_cn',))
    def test_recent_move_choices_and_assist_fixes_survive(self):
        self.assertTrue(geese.MOVESET['bounds']['My_623B2'])
        self.assertNotIn('My_623B',geese.MOVESET['bounds'])
        self.assertEqual(geese.MOVESET['air_reactions']['My_WPF'],(40,'J8_jprk00'))
        self.assertEqual(akuma.MOVESET['startup_trim'],{'Mx_3lrp':4,'Mx_66rk':4})
        self.assertEqual(akuma.MOVESET['recovery_trim'],{'Mx_3rplp':4,'Ry_shinhadouken':2})
        self.assertEqual(kunimitsu2.MOVESET['buffered_stance_transitions'],{'Kt_rk00F':'Kt_shoryu02'})
        self.assertTrue(fahkumram.MOVESET['bounds']['Nc_3rplp'])
        self.assertFalse(noctis.MOVESET['bounds']['Mz_t_srk03F'])

    def test_super_recovery_trim_keeps_shared_rows_and_projectile_timing(self):
        original = dict(direction=0x8000, button=0, req=0, extradata=0,
                        win_start=135, win_end=32767, start_frame=135, move_id=0x8001, option=0)
        tables = {'cancels': [original],
                  'moves': [dict(cancel=0, first_active=81, last_active=81, extraprops=0),
                            dict(cancel=0, first_active=81, last_active=81, extraprops=0)],
                  'extra_move_properties': [dict(frame=80, id=0x8168, value=14)],
                  'projectiles': [dict(contacts=5, speed=120)]}
        before = copy.deepcopy(tables)
        converter = types.SimpleNamespace(T=tables, newidx={0:0}, kn={'Ry_shinhadouken':0},
                                          stats=collections.Counter())
        trim_recovery(converter, 'Ry_shinhadouken', akuma.MOVESET['recovery_trim']['Ry_shinhadouken'])
        end = tables['cancels'][tables['moves'][0]['cancel']]
        self.assertEqual((end['win_start'],end['start_frame'],end['win_end']), (133,133,32767))
        self.assertEqual(tables['cancels'][0], before['cancels'][0])
        self.assertEqual(tables['moves'][1], before['moves'][1])
        self.assertEqual(tables['moves'][0]['first_active'], 81)
        self.assertEqual(tables['moves'][0]['last_active'], 81)
        self.assertEqual(tables['extra_move_properties'], before['extra_move_properties'])
        self.assertEqual(tables['projectiles'], before['projectiles'])
