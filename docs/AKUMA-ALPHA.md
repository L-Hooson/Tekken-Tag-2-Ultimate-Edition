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
| Runtime hitstop adapter | Experimental Shinku and EX opt-ins | **Fresh build/game test pending** |
| Reversible Shinku native inter-hit gate | Opt-in only; implemented and wired | **Fresh Windows build/game test pending** |
| EX Hadoken second-hit reaction overlay | Verified reversible four-ID component; **not wired to live runtime** | Standalone C++ test passed locally |
| EX Hadoken: two-hit nine-tick gate / 8+5f stop | Optional runtime implementation; knockdown/pushback still missing | **Fresh build/game test pending** |
| Super Fireball: five-hit per-impact hitstop | Experimental 6/6/6/6/5 runtime path | **Fresh build/game test pending** |
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
  **connected only for the experimental Shinku runtime path**, using
  post-native-dispatch contact classification for both Shinku and EX (separate
  default-off environment flags). General imported contact-stop actions are
  not connected. A fresh runtime test is still required.
- `runtime/src/title/akuma_projectile_timing.h` preserves the approved
  EX Hadoken 8/5-frame stops at nine-tick spacing and Super Fireball
  6/6/6/6/5-frame stops at seven-tick spacing, without writing guest memory.
  Its source-only C++ test passed locally. Shinku's timing values are now
  consumed by the opt-in contact callback; EX also has a separately enabled
  native nine-tick gate and 8/5-frame stop. The EX knockdown/pushback overlay
  has not yet been connected.
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

## Phase 82E: opt-in reversible native Shinku projectile gate

The existing Phase 82C read-only contact probe now also supports an
**experimental native gate**, explicitly disabled unless
`TTT2_TEST_AKUMA_SUPER_GATE=1` is set. The original
`TTT2_TRACE_AKUMA_CONTACT=1` diagnostic remains read-only.

- After native damage dispatch, only Akuma's verified
  `Ry_shinhadouken` five-contact projectile may arm the gate.
- The first four accepted contacts temporarily zero native projectile speed
  and enemy target mask; after **seven battle ticks**, the original values
  are restored if the slot, row, and temporary values still match.
- The verified projectile position is held around the native fighter-physics
  call during the gated interval, matching the proven experimental approach.
- Fighter swaps, slot retirement or reuse, changed memory, failed writes,
  and mismatched native hit counters are handled conservatively. A failed
  validation never writes restored values into a replacement row.
- The final (fifth) hit is not gated. The normal EX Hadoken probe remains
  read-only unless the new separate `TTT2_TEST_AKUMA_EX_RUNTIME=1` flag is set.
- `runtime/src/title/akuma_reversible_gate.h` contains the small reversible
  component; `tests/akuma_reversible_gate_test.cpp` covers arm/restore,
  position hold, recycled slots, unexpected memory changes and partial
  native write failure. Its standalone C++ test compiled and passed locally.

The additional `TTT2_TEST_AKUMA_SUPER_RUNTIME=1` opt-in enables the
seven-tick native gate **and** the defender's per-contact 6/6/6/6/5-frame
animation/physics stop through the experimentally proven title clock hooks.
These two clocks are independent of the 71-frame Super cinematic.
`TTT2_TEST_AKUMA_SUPER_GATE=1` alone still enables gate-only testing.
No opt-in is enabled for ordinary matches.

**Not yet implemented:** EX Hadoken's knockdown reaction and second-hit
pushback integration, general T7 contact action dispatch, and production-default
hitstop behaviour. The complete Windows game build and clean-import
in-game validation remain pending. Do not replace the working local
Phase 82A installation with this source branch yet.

## Phase 82F: EX two-hit gate and repeat-projectile lifecycle

- The E11 normal/EX Hadoken pilot is integrated behind a **separate**
  `TTT2_TEST_AKUMA_EX_RUNTIME=1` runtime switch. This is **off by default**.
- Confirmed native EX Hadoken contact must satisfy the observed E11 projectile
  layout (capacity 2, speed 100, enemy mask 1), Akuma ownership, the active
  opponent and accepted native damage flags. Only then can the reversible
  gate temporarily stop native projectile motion and targeting for **nine
  battle ticks**, restoring the verified values on release or a context change.
- The approved two-impact defender freeze values **8f then 5f** are scheduled
  through the existing per-fighter hitstop adapter. This does **not** yet
  implement the tested second-hit knockdown/reaction-table substitution or
  pushback: the live EX path is still incomplete.
- A completed EX or Super sequence can now re-arm on a fresh full-capacity
  first-contact counter after the applicable 9- or 7-tick interval, including
  when the native engine reuses the same projectile slot and row. Contact
  repeat tests were added to the pure timing model.
- The selected original T7 contact actions remain **disabled** for Akuma until
  a separate verified native property adapter is ready. Do not enable
  `HITSTOP_CONTACT_ACTIONS` merely to enable the EX/Super collision pilot.
- `runtime/src/title/akuma_ex_pushback.h` is a **standalone**, reversible
  seven-pointer pushback overlay extracted from the accepted Phase 80E09
  experiment. It validates the expected source reaction 2516–2519, native
  stage-two layout (five 0x50-byte reaction records earlier), and pushback
  values (front 48/30, downed 32/50 with value 8). The guest accessor is
  injected, so tests use synthetic memory and no game files. Its local
  C++ test passed on apply, restore, third-party conflict and partial-write
  recovery; Windows and Linux CMake/CTest and source CI include the test.
  **This is not connected to live runtime memory**. Stage-two knockdown and
  pushback in a fresh game build remain unverified.
- `.github/workflows/akuma-source-checks.yml` adds source-only GitHub checks
  for aerial Tatsu, conversion opt-ins, EX reaction overlay, the two projectile
  timing models, and hitstop scheduling. They require no redistributed game
  files. Workflow execution or success is **not yet confirmed**.

### Safe development switches

| Variable | Effect | Default |
| --- | --- | --- |
| `TTT2_TRACE_AKUMA_CONTACT=1` | Read-only projectile logs | Off |
| `TTT2_TEST_AKUMA_SUPER_GATE=1` | Reversible Shinku seven-tick gate only | Off |
| `TTT2_TEST_AKUMA_SUPER_RUNTIME=1` | Shinku gate plus 6/6/6/6/5f stops | Off |
| `TTT2_TEST_AKUMA_EX_RUNTIME=1` | EX gate plus 8/5f stops; no reaction rewrite | Off |
| `TTT2_TEST_AKUMA_NEUTRAL_METER=1` | Practice-only idle refill | Off |

**None of the new EX/Super runtime features has been end-to-end validated in
a fresh native game build.** Keep the previously tested Phase 82A installation
untouched and do not make the experimental switches the default.

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
