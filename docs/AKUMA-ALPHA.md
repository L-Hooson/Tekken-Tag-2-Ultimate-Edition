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
| Runtime hitstop adapter | **Not yet ported** | Pending |
| EX Hadoken: two-hit reaction/knockdown/pushback | **Not yet ported** | Pending |
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

Windows and Linux CMake/CTest targets are included as `hitstop_model_test`.
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
