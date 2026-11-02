# Akuma v0.1.0-alpha.1 — reproducibility gate

**Status:** source-only tests are passing in CI on earlier commits. Complete
Windows source-build and **fresh** game import remain necessary before the
alpha can be described as reproducible. Do not confuse the existing
experimental Phase 82A game installation with a generated build.

## What a clean source-built Akuma must demonstrate

1. **Source identity:** record one exact commit SHA from
   `feature/akuma-v0.1.0-alpha`. Test from a clean checkout or pinned
   GitHub source archive. Do not copy the experimental Phase 82A
   `.moveset` or compiled runtime into the fresh checkout.
2. **Source tests:** run
   `test_aerial_tatsu.py`, `test_akuma_projectiles.py`,
   `test_contact_stop_actions.py`, and
   `test_akuma_pack_validator.py` with `unittest discover`.
   Build all dedicated C++ Akuma tests. Check the GitHub Actions runs for the
   same commit; queued builds are not passes.
3. **User-owned input:** import from the owner's extracted Tekken 7 game
   files and legally obtained Wii U TTT2 game using
   `tools/characters/import_character.py akuma --game <TTT2 folder>
   --movesets <T7 extracted movesets> --out <isolated characters folder>
   --outfits none`. The folder must be separate from the working Phase 82A
   installation; no game data is permitted in the repository.
4. **Binary invariants:** after import, run:
   `python tools/characters/tests/verify_akuma_alpha_pack.py
   <isolated characters/mrx/mrx.moveset> --report <local report.json>`.
   It checks all six aerial Tatsu entry routes, EX two-hit projectile
   count, and Super five-hit projectile count. This proves the imported
   pack's **static** structure, not full frame-accurate combat.
5. **Native game:** build and launch the runtime **from this same source
   commit** using the Windows launcher and the owner's Wii U archive.
   Make it load the newly imported pack, not the earlier experimental one.
   Verify six jump-state normal/EX Tatsumakis; EX Hadoken 2 hits with
   standing first impact, knockdown and pushback on the second;
   Shinku Hadoken 5 separate impacts with 6/6/6/6/5f freeze and 7-tick
   inter-hit gate; and safe cleanup of all temporary projectile gates on
   tags, scene exits, and repeat shots.
6. **Opt-ins and isolation:** new EX and Super runtime behaviour is still
   behind test flags; check both enabled and disabled paths in Practice.
   Only use `TTT2_TEST_AKUMA_EX_STAGE2=1` alongside
   `TTT2_TEST_AKUMA_EX_RUNTIME=1`. Normal gameplay must not be affected
   by experiments when flags are unset. Keep the local Phase 82A
   installation and original character packs unchanged.
7. **Evidence:** keep exact commit SHA, compiler/CI result, importer log,
   `verify_akuma_alpha_pack.py` JSON, and a fresh gameplay log/video.
   Share only text reports and observations, not extracted game files.

## Source-only tests versus end-to-end acceptance

The repository's regression CI intentionally uses synthetic rows because it
must never host proprietary Tekken binaries. A passing CI run **cannot**
certify a native Windows game launch or reproduced combat. The reproducibility
gate is complete only when a fresh import and gameplay comparison also pass.
If the clean importer or game build fails, retain the logs, address the issue
in this feature branch, and rerun the gate on a newly pinned SHA.

The ChatGPT-assisted Windows `Akuma-Alpha-Clean-Import-Repro-OneClick.ps1`
script automates steps 1–4 in an isolated workspace and produces a
reports-only ZIP. It optionally opens the clean launcher for step 5.
The source script is separately distributed in the development conversation,
not included in this repository. The script is intended for the owner's
Windows machine and has not been executed by GitHub CI.
