# Reproduce and validate the Akuma candidate

A passing CI run establishes source regressions only. Release acceptance
requires a clean runtime build, fresh deterministic import and measured
Practice parity. The present assessment is **source-build-only**.

## Isolated Windows run

Use PowerShell with Git, Python (NumPy/Pillow), the documented Windows
LLVM-MinGW/CMake/Ninja toolchain, and your own TTT2 and T7 source files.
The compiler/dependency versions and hashes are recorded in
`tools/windows-bootstrap.lock.json`. Supply the toolchain folder used by
`tools/build_windows_runtime.ps1`; the validator checks it before building.
Allow at least 8 GB on the validation drive. The workspace must not exist.

```powershell
.\tools\validate_akuma_windows.ps1 `
  -Commit '<full 40-character candidate commit SHA>' `
  -Workspace 'E:\Akuma-Validation' `
  -ToolsRoot 'E:\Windows-Tools' `
  -Game 'E:\My-TTT2-Install' `
  -Rpx 'E:\My-TTT2-Source\code\Tekken.rpx' `
  -Tekken7 'E:\SteamLibrary\steamapps\common\TEKKEN 7' `
  -Movesets 'E:\My-Extracted-T7-Movesets' `
  -Launch -Experiments
```

Paths above are examples. `-SaveSeed` may name a save directory to copy into
the isolated test save. Original saves and game installations are read-only
inputs. Omit `-Experiments` to exercise the default path first.

The script pins a new checkout to the supplied SHA, clears inherited test
flags, isolates settings/saves, fetches checked dependencies, regenerates
native source from the RPX, builds the Windows runtime, runs CTest and the
Python suite, and imports two fresh packs. It compares complete sorted
SHA-256 manifests, checks the generated moveset, records input/build hashes,
and optionally launches the fresh runtime. It never installs over a game.
`Rollback.ps1` retains the test save and restores its local backup.
`feedback-reports.zip` contains only logs, hashes and status reports.
A successful launch is explicitly **not** a gameplay pass.

## Individual import and static verification

```powershell
python tools/characters/import_character.py akuma `
  --game '<TTT2 source or install>' --tekken7 '<T7 install>' `
  --movesets '<own extracted movesets>' --out '<new private pack folder>' `
  --outfits none
python tools/characters/tests/verify_akuma_alpha_pack.py `
  '<new private pack folder>/mrx/mrx.moveset' --report '<local report.json>'
```

The verifier checks 36 restored routes across six states, EX two contacts,
Super five contacts, and exclusive EX stage-two reaction ownership. It does
not prove frame windows, collision behaviour, physics or visual fidelity.

## Record gameplay separately

Record the candidate SHA, executable/pack hashes, enabled flags, input route,
source reference result, fresh-build result, tick spacing and freeze duration.
Exercise both sides, hit/block/whiff, ordinary matches with meter refill off,
repeated shots, interruption, round resets, tags, Tag Assault and other fighters.
Keep exact failures and untested cases visible. See the current limitations
in `AKUMA-ALPHA.md` before deciding whether release criteria are met.

Only source and asset-free reports may be shared. Never commit game assets,
`.tkmvst`, generated `.moveset`, generated game C++, or game executables.
