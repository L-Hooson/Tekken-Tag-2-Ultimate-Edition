"""Tekken 7's 2D (Street Fighter style) jumps and air moves -> TTT2.

Tekken 7 moves a few fighters (Akuma, Eliza; also Geese and Noctis) through the air by physics instead of by
their animations: move property 0x8226, value bytes (A, B, C, 0), starts an arc with take-off speed A mm/frame and
gravity B/10 mm/frame^2. C is an initial height in units of 10 mm, NOT horizontal speed; zero selects the current
sampled animation height. B = 60 is TTT2's own gravity (6.0). It fires on the frame the jump's
animation reaches its flat airborne pose (frame 9 neutral, 5 forward and back); the animation then holds that pose
(its hips stay at one height) and the move holds its last frame (airborne_end == ground_fall == the last frame)
until the arc lands, then its landing cancel runs. An air move cancelled out of the jump (0x8227 at frame 0x8001:
air normals, air specials) continues the arc; one with its own 0x8226 (a dive kick: A = 0, B = 120..220) starts a
new one where the fighter is.

TTT2 has neither (its property interpreter does nothing with 0x8226; its jumps carry their arcs in the animation's
hips channel, the fighter's own height stays 0), so:

* legacy ground arcs (the ordinary jumps) play a new raw animation: the Tekken 7
  animation with the arc added to the hips height from the 0x8226 frame on (n = frame - f0: y = n A - g n (n + 1)
  / 2, the sum TTT2's physics would make), extended until the arc lands by repeating the held frame; its
  airborne_end, ground_fall and landing cancels move by the frames added. Forward and back jumps whose Tekken 7
  animations are deltas ("_sabun" over the neutral jump; Akuma) get the delta made absolute first: its rotations
  are absolute, its hips height between the take-off and the last two frames is relative to the neutral jump's
  air height. Such a move also gets 0x822D at frame 0x8001 (value A, B, C, f0) so the runtime knows the arc when
  an air move is cancelled out of it.
* SOURCE_PHYSICS_ARCS opts Demon Flip into runtime physics instead. Its 0x822D marker fires at take-off with
  bit 7 of the low byte set. The original animation speed, ground_fall and cancel windows stay intact. The
  animation contains only the source movement mode's pose contribution; 0x822C's value holds the landing
  reference as IEEE float bits. Runtime physical Y is measured relative to that reference. The source 0x81c3
  mode 3 omits animated root Y; mode 4 adds it. Reachable air continuations get the same coordinate conversion.
* 0x8227 becomes 0x822C at frame 0x4001 (every frame from 1 on: a cancel may start a move past its frame 1), and an
  air move's 0x8226 stays as it is. The runtime (runtime/src/title/roster.cpp, "2D arcs") flies those moves with
  the fighter's own height and vertical speed, holds them on their ground_fall frame and lands them.

TTT2's property interpreter ignores 0x8226, 0x822C and 0x822D, and no TTT2 moveset uses them.
"""
import re
import struct

import numpy as np

from ..ttt2 import animation as A
from . import idmaps as M

ARC, AIR_MOVE = 0x8226, 0x8227            # Tekken 7: start an arc; this air move continues the arc
BAKED, AIR_MARK = 0x822D, 0x822C          # TTT2 (no-op ids): a baked ground arc; a 2D air move
AIR_MARK_FRAME, BAKED_FRAME = 0x4001, 0x8001
SOURCE_PHYSICS = 0x80  # low byte of BAKED: runtime arc, not a baked displacement (f0 < 128)
MOVEMENT_MODE = 0x81C3


def height(n, a, g):
    """The arc's height n frames after its start (TTT2's integration: vy -= g, then y += vy, once a frame)."""
    return n * a - g * n * (n + 1) / 2.0


def touchdown(a, g):
    """Frames from the arc's start to its landing (height <= 0)."""
    n = 1
    while height(n, a, g) > 0:
        n += 1
    return n


def params(value):
    """0x8226 -> (take-off speed, gravity, initial-height byte).

    T7 14071b6b6: C != 0 initializes physical Y to C*10 mm; C == 0 copies
    current sampled root height. It is NOT a horizontal-speed percentage.
    """
    value &= 0xFFFFFFFF
    return value >> 24, ((value >> 16) & 0xFF) / 10.0, (value >> 8) & 0xFF


def float_word(value):
    return struct.unpack('>I', struct.pack('>f', value))[0]


def landing_height(dec, ground_fall):
    """T7 14071b772: sample the combined translations at ground_fall - 1."""
    f = max(0, min(dec['frames'] - 1, ground_fall - 1))
    return float(dec['values'][f, 0, 1] + dec['values'][f, 1, 1])


def continuations(k, names):
    """Air moves reachable from opted-in roots, stopping at grounded exits.

    Follow actual cancel targets, including EX fireballs whose names do not
    share the Demon Flip prefix. Requirements remain the converter's concern.
    """
    todo = [i for i in range(len(k.tables['moves'])) if k.move_name(i) in names]
    seen, air = set(todo), set()
    while todo:
        i = todo.pop()
        for row in M.cancels(k, k.tables['moves'][i]['cancel']):
            if row['direction'] in (M.LIST_END, M.GROUP, M.GROUP_END):
                continue
            target = k.resolve_alias(row['move_id'])
            if target is None or target in seen:
                continue
            seen.add(target)
            props = M.xps(k, k.tables['moves'][target]['extraprops'])
            if any(p == AIR_MOVE for _, p, _ in props):
                air.add(target)
                todo.append(target)
    return air


def source_motion(dec, floor, *, f0=0, modes=()):
    """Render a source physical-Y arc with TTT2's additive player Y.

    The runtime stores physical Y minus the landing threshold. Mode 3 omits
    animated root Y; mode 4 adds it. Keep rotations and horizontal motion.
    No trajectory is baked: animation speed and physics have separate clocks.
    Before f0 and after touchdown the original pose is needed; the runtime
    finishes the move through its original landing cancel.
    """
    values = A.to_raw(dec['descriptors'], dec['values'], dec['type']).copy()
    desc = [7 if A.is_rotation(d) and dec['type'] != A.RAW else d for d in dec['descriptors']]
    for f in range(f0, dec['frames']):
        mode = 3
        for at, selected in modes:
            if at <= f:
                mode = selected
        if mode not in (3, 4):
            raise ValueError('unsupported source arc movement mode')
        root = float(values[f, 0, 1] + values[f, 1, 1]) if mode == 4 else 0.0
        values[f, 0, 1] = 0.0
        values[f, 1, 1] = floor + root
    return desc, values.astype(np.float32)


def is_ground_arc(air, frame):
    """An arc to bake: the move starts on the ground (not an air move) and the arc's frame is a move frame."""
    return not air and frame < 0x4000


def decode_t7(k, off):
    return A.decode(k.animbuf[off:off + k.anim_size[off]], '<')


def absolute_sabun(delta, whole):
    """A "_sabun" animation (decoded) made absolute against the whole animation it is a delta of: its hips height
    (channel 1 y) is relative to the whole's flat air height on the frames where it is small (the airborne
    frames; the take-off and the landing frames are absolute)."""
    wy = whole['values'][:, 1, 1]
    base = float(np.median(wy[wy > 0.8 * wy.max()]))
    v = delta['values'].copy()
    rel = np.abs(v[:, 1, 1]) < 0.5 * base
    v[rel, 1, 1] += base
    return dict(delta, values=v), base, int(rel.sum())


def bake(dec, f0, a, g, hold):
    """(descriptors, values (frames', nch, 3), frames added) of a raw animation: dec (decoded) with the arc from
    frame f0 added to the hips height, the frame `hold` repeated until the arc lands."""
    values = A.to_raw(dec['descriptors'], dec['values'], dec['type'])
    desc = [7 if A.is_rotation(d) and dec['type'] != A.RAW else d for d in dec['descriptors']]
    frames = dec['frames']
    hold = min(hold, frames - 1)
    ext = max(0, f0 + touchdown(a, g) - hold)
    out = np.concatenate([values[:hold + 1]] + [values[hold:hold + 1]] * ext + [values[hold + 1:]], 0).copy()
    for f in range(f0, out.shape[0]):
        out[f, 1, 1] += max(0.0, height(f - f0, a, g))
    return desc, out.astype(np.float32), ext


def whole_name(sabun_name):
    return re.sub(r'_[fb]_sabun', '', sabun_name, flags=re.I)


def encode(descriptors, values):
    """The raw TTT2 (big-endian) animation."""
    return A.encode_raw(descriptors, values, '>')


DFLIP_PAUSE_MOVES = frozenset(('Mx_hyaki_gouB', 'Mx_hyaki_gouLK', 'Mx_hyaki_gouD'))


def followup_pause_rows(name, props):
    """Retain verified T7 airborne pause events, without changing their clock encoding.

    0x8001 and 0x8008 use the separate move-event clock, not pose frame numbers.
    The runtime interprets 0x822B only for these source-clock continuations.
    """
    if name not in DFLIP_PAUSE_MOVES:
        return []
    rows = [(f, v) for f, prop, v in props if prop == 0x822B]
    if rows != [(0x8001, 1), (0x8008, 0)]:
        raise ValueError('unverified Demon Flip pause schedule: ' + name)
    return [dict(frame=f, id=0x822B, value=v) for f, v in rows]
