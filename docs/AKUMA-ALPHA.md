# Akuma v0.1.0-alpha.1

**Release assessment: source-build-only. Gameplay parity is not certified.**
The internal importer revision is **8**; it is not the public version.
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
  Non-projectile specials select their retained stop actions at a confirmed
  native melee contact, checking source windows and additional predicates.
  Only stop timings are dispatched; unrelated requirement actions are not
  replayed. This survives native move-stage transitions and repeated contacts.
  Cancelable normals separately receive a two-frame contact pause for both
  fighters to keep late contact-confirm inputs within the original move window.
  This is an opt-in adjustment following the user's execution-feel trial;
  it does not extend source cancel-window endpoints or pause whiffs.
- Preserve the existing opt-in, Practice-only neutral gauge refill.
- Apply the user's two-frame Super Fireball recovery adjustment in the importer:
  neutral return moves from frame 135 to 133. Its projectile contacts, cinematic
  pause and 6/6/6/6/5 contact stops retain their prior timing.

## Evidence from the Windows source build

On 2026-11-02 a fresh checkout regenerated 79,564 reachable functions in
257 shards from the owner's RPX and built the native Windows runtime.
The subsequent Windows CTest run passed **19/19** tests, including native game
input checks. The revision-8 importer suite passed **715 tests (28 skipped)**.
The fresh revision-8 pack passed all ten structural checks, including private
EX reaction ownership and the two-frame Super recovery adjustment.

In owner-operated Practice trials on 2026-11-03, the user confirmed normal
cancels on hit/block, their two-frame contact pause, heavy Tatsu/Shoryuken
hitstop, EX first stagger/second knockdown and hit/block projectile stops.
The user also accepted Super's shorter recovery and five impacts at close and
medium range on hit/block, all normal/EX aerial Tatsu jump directions, Demon
Flip follow-ups/landings, tagging and Tag Assault. Native frame traces record
the five Super pause lengths and frozen animation counters. These manual
acceptances do not prove exact damage/trajectory or whole-roster parity.

The source-only CI workflow runs synthetic converter and C++ tests on Linux.
Check the workflow results for the exact candidate commit; an older green run
is not evidence for a newer checkout.

## Experimental switches

All switches below are off unless explicitly set to `1` in the game process.

| Switch | Effect |
| --- | --- |
| `TTT2_TRACE_AKUMA_CONTACT` | Read-only native contact logging |
| `TTT2_TRACE_AKUMA_FRAMES` | Read-only animation/position samples around fighter updates |
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

Complete a clean build and CI for the final committed revision. Remaining
measured comparisons include damage/trajectory/pushback against Phase 82A,
meter off/on and ordinary-match isolation, whiff cancels, other projectile
interactions and unrelated fighters. Earlier trials recorded two longer first
Super intervals (8 and 13 ticks); controlled traces must resolve those against
the seven-tick reference. Animation counters stop, but small position changes
can occur between fighter updates; the frame trace must not be treated as proof
of whole-battle physics parity.

Projectile observation now retains native row identity and hit/life counters
through private current/pending scratch rows. Final contacts have been observed
in live trials. Do not infer final impacts from launch animations alone.
Reaction IDs and the five-row pushback relationship are validated assumptions
of the reviewed source layout; incompatible source data fails closed.

See [AKUMA-REPRODUCIBILITY.md](AKUMA-REPRODUCIBILITY.md) for the isolated
validation procedure. Keep the working Phase 82A installation untouched.
No generated game source, packs, dumps or executables belong in Git.
