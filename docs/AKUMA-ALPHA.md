# Akuma v0.1.0-alpha.1

**Release assessment: source-build-only. Gameplay parity is not certified.**
The internal importer revision is **7**; it is not the public version.
This branch reconstructs a candidate from the owner's original game files.
It does not require the Phase 82A executable or a patched character pack.

## Implemented source changes

- Restore 36 normal/EX aerial Tatsumaki routes across six jump states.
  Translate the source airborne requirement into the bounded native airborne
  window; require 500 gauge for EX. Unknown requirements fail the import.
- Reconstruct EX Hadoken's two native contacts from its source projectile chain.
  Give its stage-two reaction overlay a private six-row block, retaining the
  reviewed relative pushback source. Other moves retain their original rows.
- Use a temporary per-shot copy for the nine-tick EX and seven-tick Super
  targeting/speed gates. Shared projectile rows are never modified. Release
  both native current/pending pointers on retirement, reuse and before situation
  setup. Keeping these pointers equal prevents native stage initialization
  from resetting the projectile's remaining contacts and lifetime.
- Keep EX 8/5 and Super 6/6/6/6/5 contact-stop scheduling separate from the
  cinematic pause, on both hit and block. The EX second-hit reaction overlay
  arms only after the first native stagger has committed; blocked contacts
  never arm it. These runtime pilots remain opt-in.
- Import source-height Demon Flip C/D physics and airborne continuations,
  preserving source landing and cancel timing. Optional runtime follow-up
  and root-height corrections remain opt-in.
- Keep Akuma special-command capture at its original source end frame rather
  than extending it to delayed dispatch. Use an Akuma-only hit-or-block latch;
  the generic condition used previously also accepted Tag, not block.
  Sample accepted contact events after native battle processing for marked
  normal moves, within their source window and only if the move did not change.
- Preserve T7 contact-stop actions for the scoped, default-off runtime pilot.
  Reset scheduling on fighter identity changes; keep native input and cancel
  evaluation running while animation/physics clocks are stopped.
- Preserve the existing opt-in, Practice-only neutral gauge refill.

## Evidence from the Windows source build

On 2026-11-02 the fresh checkout regenerated 79,564 reachable functions in
257 shards from the owner's RPX and built the native Windows runtime.
The current Windows CTest run passed **19/19** tests, including native game
input checks. The full importer suite passed **713 tests (28 skipped)**.
The fresh revision-7 pack passed all nine structural checks, including private
EX reaction ownership. The source-built runtime launched with Vulkan and
the user confirmed its window visible and responding. Automated Windows
capture timed out, so no controlled Practice observation was completed.
These are build/import observations, not evidence of combat parity.

The source-only CI workflow runs synthetic converter and C++ tests on Linux.
Check the workflow results for the exact candidate commit; an older green run
is not evidence for a newer checkout.

## Experimental switches

All switches below are off unless explicitly set to `1` in the game process.

| Switch | Effect |
| --- | --- |
| `TTT2_TRACE_AKUMA_CONTACT` | Read-only native contact logging |
| `TTT2_TEST_AKUMA_EX_RUNTIME` | EX nine-tick gate and 8/5-frame defender stops |
| `TTT2_TEST_AKUMA_EX_STAGE2` | EX second knockdown/pushback; requires EX runtime |
| `TTT2_TEST_AKUMA_SUPER_RUNTIME` | Super seven-tick gate and 6/6/6/6/5 stops |
| `TTT2_TEST_AKUMA_SUPER_GATE` | Super gate alone |
| `TTT2_HITSTOP_LP_HEAVY` | Scoped general contact-action pilot |
| `TTT2_HITSTOP_SPECIALS` | Extend the general pilot to reviewed specials |
| `TTT2_TEST_DFLIP_FOLLOWUP_FIX` | Follow-up physics correction |
| `TTT2_TEST_DFLIP_ROOT_HEIGHT` | Source root-height correction |
| `TTT2_TEST_DFLIP_TAKEOFF_SYNC` | Source takeoff synchronization |
| `TTT2_TEST_AKUMA_NEUTRAL_METER` | Fill gauge only while idle in Practice |

## Outstanding release criteria

Controlled Practice measurements must compare this fresh import against
Phase 82A: all six normal/EX jump routes, landing/trajectories/damage, both
EX contacts and pushback, five Super impacts, Demon Flip continuations,
normal-to-special cancels on hit/block/whiff, meter off/on, repeated shots,
other projectile interactions, tagging, Tag Assault and unrelated fighters.

The native contact observer scans live projectile slots after damage dispatch.
If the final contact retires its slot before observation, the final stop can
be missed. This requires a native trace and a verified collision-lifetime
adapter before release. Do not infer final impacts from launch animations.
Reaction IDs and the five-row pushback relationship are validated assumptions
of the reviewed source layout; incompatible source data fails closed.
The general contact-action ledger also needs live multi-hit validation.

See [AKUMA-REPRODUCIBILITY.md](AKUMA-REPRODUCIBILITY.md) for the isolated
validation procedure. Keep the working Phase 82A installation untouched.
No generated game source, packs, dumps or executables belong in Git.
