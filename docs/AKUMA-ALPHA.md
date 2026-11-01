# Akuma v0.1.0-alpha.1 — source integration status

This is a **work-in-progress branch**, not an installable character release.
The intended target is the tested Phase 82A experimental gameplay, rebuilt
through the project's own importer and runtime without redistributing game data.

## Source-port milestones

| Change | In this branch | Fresh-import/in-game validation |
| --- | --- | --- |
| Aerial normal/EX Tatsumaki jump routes | Yes; `convert/aerial_tatsu.py` | **Pending** |
| Aerial Tatsu synthetic regression tests | Yes | Four locally exercised with supporting converter modules |
| Practice-only Akuma neutral gauge refill | Yes; environment-gated | **Pending** |
| Hitstop frame scheduler | Yes; pure C++ model | Standalone C++ test built and passed locally |
| Approved EX/Super impact cadence (pure model) | Yes; `akuma_projectile_timing.h` | C++ test passed locally |
| T7 contact-stop actions | Converter support gated, disabled in Akuma definition | Python regression tests committed; not yet executed |
| Read-only native projectile collision probe | Wired into roster/practice under `TTT2_TRACE_AKUMA_CONTACT=1` | Windows runtime verification **pending** |
| Runtime hitstop adapter | **Not yet ported** | Pending |
| EX Hadoken second-hit reaction overlay | Verified reversible four-ID component; **not wired to live runtime** | Standalone C++ test passed locally |
| EX Hadoken: complete two-hit reaction/knockdown/pushback | **Not yet ported** | Pending |
| Super Fireball: five-hit per-impact hitstop | **Not yet ported** | Pending |
| Source-physics Demon Flip/follow-up conversions | **Not yet ported** | Pending |
| Normal-to-special cancellation parity | Requires further review | Pending |
| Tag/swap robustness and projectile VFX | Requires further review | Pending |

## What is implemented now

- `tools/characters/ttt2char/convert/aerial_tatsu.py` restores 36 source-derived
  input routes across neutral, forward and backward jumps and their recovery
  states. It keeps the converted move's animations, damage, hit and landing
  properties from the player's own Tekken 7 data.
- `tools/characters/ttt2char/characters/akuma.py` opts into those routes.
  Revision **5** deliberately avoids conflicts with experimental revisions
  3 (Demon Flip source physics) and 4 (contact-stop actions). **Revision
  number alone does not mean those experimental features are integrated.**
- `runtime/src/title/gauge.cpp` includes an optional Practice testing aid:
  `TTT2_TEST_AKUMA_NEUTRAL_METER=1`. It fills Akuma's gauge only when
  both fighters are idle in a Practice round; it is off by default and
  does not alter normal-match gameplay.
- `runtime/src/title/hitstop_model.h` is a pure battle-tick timing model
  with explicit fighter lifecycle and bounded frame scheduling. It is
  **not yet connected** to the runtime's contact callback. The imported
  per-move actions must also be preserved before hitstop can function.
- `runtime/src/title/akuma_projectile_timing.h` preserves the approved
  EX Hadoken 8/5-frame stops at nine-tick spacing and Super Fireball
  6/6/6/6/5-frame stops at seven-tick spacing, without writing guest memory.
  Its source-only C++ test passed locally; it is not wired to damage callbacks.
- `convert/moveset.py` can preserve raw Tekken 7 action IDs 0x8292/0x8293
  behind `HITSTOP_CONTACT_ACTIONS`, which deliberately remains disabled
  in Akuma's character definition until the runtime adapter exists.

## Phase 82D: corrections and EX reaction overlay

- Correct the collision probe function definitions to use `cafe::title`,
  matching `roster.h` and their callers in `practice.cpp`.
- Identify the optional Practice meter-refill recipient by the pack's
  actual `code` (`mrx`), not the installation folder's filename.
- Correct aerial-Tatsu Python test imports for CTest's build-directory
  working directory.
- Isolate the Phase 80E second-hit EX Hadoken reaction substitution in
  `akuma_ex_reaction.h`. All four stagger IDs (2504–2507) must match
  before changing them to knockdown IDs (2532–2535). Restoration
  refuses unknown external writers and can recover a failed partial write.
  `akuma_ex_reaction_test` compiled and passed locally.

**This transaction is not connected to guest memory** in the clean branch.
The EX pushback pointer transfer, live projectile gate, hitstop event adapter
and full Windows import still require testing.

## Phase 82C: tracing a fresh source-built Akuma

The clean branch now brackets the native fighter simulation with
`akuma_contact_probe_begin/end` and observes accepted projectile damage
after `sub_03AE1704_orig`. The probe requires all of: a selected Akuma
owner, the currently active opponent, verified native projectile-hit flags,
`Ry_EX_hadoh` or `Ry_shinhadouken`, a live native projectile slot, the
expected hit capacity (2 or 5), and a decremented hit counter. The Super
probe additionally requires the tested native speed/team mask (120/1).

Set `TTT2_TRACE_AKUMA_CONTACT=1` in the launched game's environment to
opt into **read-only** `akuma-contact-probe` log lines. No projectile
speed, team mask, knockdown or reaction row is modified; the probe never
freezes an animation clock or changes match damage. A final projectile
contact may be unobservable after native slot retirement; this is a
**diagnostic**, not final gameplay parity. Fighter changes reset both
observation sequences.

The standalone contact classifier is covered by `akuma_contact_gate_test`
(Windows/Linux CMake/CTest). It passed a local C++ compile/run; **the full
Windows runtime build and live clean-import trace remain unverified**.

## Tested experimental reference (not committed)

The user's Phase 82A Windows build, based on Phase 81D runtime, demonstrated
normal/EX jumping Tatsu and the approved Shinku Hadoken rhythm (first four
impacts at six frames, fifth at five, seven battle ticks between impacts).
The earlier EX Hadoken experiment used separate first/second-hit reactions
and a nine-tick inter-hit gate. These claims apply to the **experimental
local installation**, **not** to a fresh build of this branch.

Current production integration needs both the original game importer
source and **targeted** runtime adapter changes. Do not copy the entire
experimental `roster.cpp`: it includes test-only instrumentation,
temporary memory overlays and game-layout assumptions that need
validation and lifecycle guards before upstream review.

## Validate without game assets

Run the source-only Python regression tests:

```bash
python -m unittest discover -s tools/characters/tests -p 'test_aerial_tatsu.py' -v
```

Run the standalone hitstop model test:

```bash
c++ -std=c++20 -Wall -Wextra -Werror -I runtime/src tests/hitstop_model_test.cpp -o hitstop_model_test
./hitstop_model_test
```

Source-only projectile timing and converter opt-in tests are also added:

```bash
c++ -std=c++20 -Wall -Wextra -Werror -I runtime/src tests/akuma_projectile_timing_test.cpp -o akuma_projectile_timing_test
./akuma_projectile_timing_test
python -m unittest discover -s tools/characters/tests -p 'test_contact_stop_actions.py' -v
```

Windows and Linux CMake/CTest include `hitstop_model_test`,
`akuma_projectile_timing_test`, `akuma_contact_gate_test` and
`akuma_ex_reaction_test`.
The latest native contact classifier can also run without game assets:

```bash
c++ -std=c++20 -Wall -Wextra -Werror -I runtime/src tests/akuma_contact_gate_test.cpp -o akuma_contact_gate_test
./akuma_contact_gate_test
```

The new C++ timing tests passed locally; Python converter CI validation
remains outstanding. The four-field EX reaction test is also available:

```bash
c++ -std=c++20 -Wall -Wextra -Werror -I runtime/src tests/akuma_ex_reaction_test.cpp -o akuma_ex_reaction_test
./akuma_ex_reaction_test
```
The full game import, runtime compile, Windows play test, and end-to-end
feature parity are **not** confirmed.

## Safety and release hygiene

- Keep `main` unchanged until clean import and regression testing.
- Never commit a Tekken 7 `.tkmvst`, extracted TTT2 assets, `.moveset`,
  generated game C++, game executables, or captured binary data.
- A draft upstream PR should be opened only after the feature works in a
  fresh import, not merely because a synthetic table test passes.
- Preserve the user's experimental Phase 82A installation separately as
  the reference, and use the project's `tools/characters` importer to
  build packs from legally obtained files on each user's machine.
