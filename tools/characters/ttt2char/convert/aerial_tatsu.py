"""Restore Akuma's Tekken 7 airborne Tatsumaki input routes during conversion.

The converter already retains the six aerial Tatsu attacks and their assets, but
its converted jump states can lack the source motion-input cancels. Reconstruct
those cancels by *name and source input*, not output table indices or pack bytes.

This is gated by MOVESET['aerial_tatsu_jumps']; other fighters are untouched.
"""

from . import idmaps as M

# (TTT2 jump move name, T7 source of the routes, T7 normal target, T7 EX target)
# The basic neutral jump's route comes from the neutral recovery-jump source,
# whose special-move window is 9..41 in Akuma's T7 moveset.
JUMP_ROUTES = (
    ('Mx_sJUMP_00_',  'Mx_sJUMPr00_', 'Mx_tatumaki_air',  'Mx_tatumaki_airEX'),
    ('Mx_sJUMP_00F',  'Mx_sJUMP_00F', 'Mx_tatumaki_airF', 'Mx_tatumaki_airEXF'),
    ('Mx_sJUMP_00B',  'Mx_sJUMP_00B', 'Mx_tatumaki_airB', 'Mx_tatumaki_airEXB'),
    ('Mx_sJUMPr00_', 'Mx_sJUMPr00_', 'Mx_tatumaki_air',  'Mx_tatumaki_airEX'),
    ('Mx_sJUMPr00F', 'Mx_sJUMPr00F', 'Mx_tatumaki_airF', 'Mx_tatumaki_airEXF'),
    ('Mx_sJUMPr00B', 'Mx_sJUMPr00B', 'Mx_tatumaki_airB', 'Mx_tatumaki_airEXB'),
)


def _cancel_list(rows, start):
    """A whole native TTT2 cancel list, including its terminating row."""
    if start is None or not 0 <= start < len(rows):
        raise ValueError('Missing jump cancel list')
    out = []
    for row in rows[start:]:
        out.append(dict(row))
        if row['direction'] == M.LIST_END:
            return out
    raise ValueError('Unterminated jump cancel list')


def _source_routes(k, source_name, normal_name, ex_name):
    name_to_id = {k.move_name(i): i for i in range(len(k.tables['moves']))}
    if source_name not in name_to_id:
        raise ValueError('Missing T7 jump: ' + source_name)
    jump = k.tables['moves'][name_to_id[source_name]]
    source = M.cancels(k, jump['cancel'])
    ex_rows = [r for r in source if r['direction'] >= 0x800D and
               k.move_name(r['move_id']) == ex_name]
    if len(ex_rows) < 2:
        raise ValueError('Expected two T7 EX aerial Tatsu commands: ' + source_name)
    ex_rows = ex_rows[:2]
    ex_inputs = {r['direction'] for r in ex_rows}
    normal_rows = [r for r in source if r['direction'] >= 0x800D and
                   k.move_name(r['move_id']) == normal_name and
                   r['direction'] not in ex_inputs]
    if len(normal_rows) < 4:
        raise ValueError('Expected four T7 normal aerial Tatsu commands: ' + source_name)
    return ex_rows + normal_rows[:4]


def apply(converter):
    """Append source-derived Tatsu routes ahead of each original jump cancel list.

    Safe to call twice. Original jump cancels, animation fields, landing rows,
    damage and gauge properties are retained. No binary moveset is embedded.
    """
    if not converter.defn.MOVESET.get('aerial_tatsu_jumps'):
        return 0
    if converter.game != 't7':
        raise ValueError('Aerial Tatsu routes currently require a Tekken 7 source')

    T = converter.T
    output_ids = {converter.move_name(i): i for i in range(len(T['moves']))}
    needed = {name for jump, _, normal, ex in JUMP_ROUTES for name in (jump, normal, ex)}
    missing = needed - set(output_ids)
    if missing:
        raise ValueError('Converted Akuma moves missing: ' + ', '.join(sorted(missing)))

    added = 0
    for jump_name, source_name, normal_name, ex_name in JUMP_ROUTES:
        jump_id = output_ids[jump_name]
        jump = T['moves'][jump_id]
        original = _cancel_list(T['cancels'], jump['cancel'])
        # A repeat invocation must not duplicate the input routes.
        existing = [r for r in original if r['move_id'] in
                    (output_ids[normal_name], output_ids[ex_name]) and r['direction'] >= 0x800D]
        if existing:
            if len(existing) == 6:
                continue
            raise ValueError('Partially restored aerial Tatsu routes on ' + jump_name)

        source_rows = _source_routes(converter.k, source_name, normal_name, ex_name)
        routes = []
        for src in source_rows:
            src_name = converter.k.move_name(src['move_id'])
            req, ok = converter.conv_reqs(src['req'])
            if not ok or req is None:
                raise ValueError('T7 aerial Tatsu requirement failed conversion: ' + source_name)
            direction = 0x800D + converter.conv_seq(src['direction'] - 0x800D)
            routes.append(dict(direction=direction, button=src['button'], req=req,
                               extradata=converter.extradata(src['extradata']),
                               win_start=src['win_start'], win_end=src['win_end'],
                               start_frame=src['start_frame'], move_id=output_ids[src_name], option=0))
        if len(routes) != 6:
            raise ValueError('Incorrect aerial Tatsu cancel count')
        jump['cancel'] = len(T['cancels'])
        T['cancels'].extend(routes + original)
        added += len(routes)

    converter.stats['Akuma airborne Tatsu cancels restored'] += added
    return added
