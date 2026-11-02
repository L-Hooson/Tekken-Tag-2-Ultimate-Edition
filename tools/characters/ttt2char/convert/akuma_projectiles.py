"""Reconstruct Akuma's native two-hit EX Hadoken from his T7 projectile chain.

T7 emits two successive projectile rows. Its stage-one transitions use
conditional requirements other than the generic projectile-hit condition,
so the generic chain counter returns one. Identify the source by move name
and spawn property, never by a fixed table index.
"""
import struct
from . import idmaps as M

EX_MOVE = 'Ry_EX_hadoh'
EX_SPAWN = 0x820b


def ex_hadoken_hits(k, projectile_index):
    """Return 2 for a validated T7 EX first stage, None for other projectiles.

    Unexpected EX source layouts raise ValueError instead of silently
    creating incorrect game data.
    """
    matches = [row for i, row in enumerate(k.tables['moves'])
               if k.move_name(i) == EX_MOVE]
    if len(matches) != 1:
        raise ValueError('Akuma requires exactly one T7 EX Hadoken move')
    spawns = [value for _, prop, value in M.xps(k, matches[0]['extraprops'])
              if prop == EX_SPAWN]
    if len(spawns) != 1:
        raise ValueError('EX Hadoken must spawn one source projectile')
    if projectile_index != spawns[0]:
        return None
    source = k.tables['projectiles']
    if not 0 < projectile_index < len(source):
        raise ValueError('EX projectile source index outside table')
    first = source[projectile_index]
    transitions = {c['move_id'] for c in M.cancels(k, first['cancel'])
                   if c['option'] & 0x40 and c['move_id'] not in (None, 0)}
    if len(transitions) != 1:
        raise ValueError('EX projectile source does not have one next stage')
    second_index = transitions.pop()
    if not 0 < second_index < len(source) or second_index == projectile_index:
        raise ValueError('EX projectile next stage invalid')
    second = source[second_index]
    raw_first = struct.unpack('<42I', first['raw'])
    raw_second = struct.unpack('<42I', second['raw'])
    if (raw_first[9] != 1 or raw_second[9] != 1 or
        raw_first[12] != 100 or raw_second[12] != 100 or
        first['hit_condition'] is None or second['hit_condition'] is None or
        first['hit_condition'] == second['hit_condition']):
        raise ValueError('EX two-stage projectile fields differ from verified T7 layout')
    return 2


def isolate_ex_reaction(tables, hc, count):
    """Give the runtime stage-two overlay an EX-only reaction row.

    Keep the verified five-row relative pushback source in the private block.
    All pushback objects remain read-only; only the private row's pointers and
    four reaction IDs may be overlaid. The hit conditions were just appended
    by conv_hitconds and must not be shared with a different move.
    """
    conditions = tables['hit_conditions']
    reactions = tables['reactions']
    if hc is None or count != 2 or hc + count != len(conditions):
        raise ValueError('EX Hadoken requires two newly converted hit conditions')
    target = conditions[hc + 1]['reactions']
    fields = ('default', 'left', 'right', 'back')
    if (target is None or not 5 <= target < len(reactions) or
        tuple(reactions[target][f] for f in fields) != (2504, 2505, 2506, 2507) or
        tuple(reactions[target - 5][f] for f in fields) != (2516, 2517, 2518, 2519)):
        raise ValueError('EX Hadoken reaction/pushback layout differs from reviewed source')
    private = len(reactions) + 5
    reactions.extend(dict(row) for row in reactions[target - 5:target + 1])
    conditions[hc + 1] = dict(conditions[hc + 1], reactions=private)
    return private
