# Builds and imports only into a NEW workspace. Never updates the game installation.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$Commit,
    [Parameter(Mandatory=$true)][string]$Workspace,
    [Parameter(Mandatory=$true)][string]$ToolsRoot,
    [Parameter(Mandatory=$true)][string]$Game,
    [Parameter(Mandatory=$true)][string]$Rpx,
    [Parameter(Mandatory=$true)][string]$Tekken7,
    [Parameter(Mandatory=$true)][string]$Movesets,
    [string]$SaveSeed,
    [switch]$Launch,
    [switch]$Experiments
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoUrl = 'https://github.com/L-Hooson/Tekken-Tag-2-Ultimate-Edition.git'
$Workspace = [IO.Path]::GetFullPath($Workspace)
if (Test-Path -LiteralPath $Workspace) { throw 'Workspace must be new; existing files are never overwritten.' }
foreach ($parameterName in @('ToolsRoot','Game','Rpx','Tekken7','Movesets')) {
    $path = (Get-Variable -Name $parameterName -ValueOnly)
    if (!(Test-Path -LiteralPath $path)) { throw "Required input missing: $path" }
    $resolved = (Resolve-Path -LiteralPath $path).Path
    Set-Variable -Name $parameterName -Value $resolved
    if ($Workspace.StartsWith($resolved.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Workspace must be outside all input directories.'
    }
}
if ($SaveSeed) { $SaveSeed = (Resolve-Path -LiteralPath $SaveSeed).Path }
foreach ($tool in @('git','python')) { Get-Command $tool -ErrorAction Stop | Out-Null }
$cmake = Join-Path $ToolsRoot 'python/cmake/data/bin/cmake.exe'
$compiler = Join-Path $ToolsRoot 'llvm-mingw-20261020-ucrt-x86_64/bin'
foreach ($path in @($cmake,(Join-Path $ToolsRoot 'python/bin/ninja.exe'),(Join-Path $compiler 'clang++.exe'))) {
    if (!(Test-Path -LiteralPath $path)) { throw "Documented Windows toolchain missing: $path" }
}
$drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($Workspace))
if ($drive.AvailableFreeSpace -lt 8GB) { throw 'Allow at least 8 GB free for the isolated build and two imports.' }
New-Item -ItemType Directory -Path $Workspace | Out-Null
$reports = New-Item -ItemType Directory -Path (Join-Path $Workspace 'reports')
$source = Join-Path $Workspace 'source'
$savedEnv = @{}
$envNames = @('PATH','LOCALAPPDATA','XDG_CONFIG_HOME','XDG_DATA_HOME','TEMP','TMP')
$envNames += @(Get-ChildItem Env: | Where-Object Name -Like 'TTT2_*' | ForEach-Object Name)
foreach ($name in $envNames) { $savedEnv[$name] = [Environment]::GetEnvironmentVariable($name,'Process') }
$status = [ordered]@{commit=$Commit; branch='feature/akuma-v0.1.0-alpha'; build='pending'; import='pending'; deterministic='pending'; gameplay='unverified'}
function Invoke-Checked([string]$Name,[string]$Program,[string[]]$Arguments) {
    & $Program @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $reports "$Name.log") | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}
function Write-Manifest([string]$Folder,[string]$Destination) {
    $entries = @(Get-ChildItem -LiteralPath $Folder -File -Recurse | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($Folder.Length+1).Replace('\','/'); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
    ConvertTo-Json -InputObject $entries -Depth 4 | Set-Content -LiteralPath $Destination -Encoding UTF8
}
try {
    foreach ($name in @($envNames | Where-Object { $_ -like 'TTT2_*' })) {
        [Environment]::SetEnvironmentVariable($name,$null,'Process')
    }
    $env:PATH = "$compiler;"+$env:PATH
    foreach ($name in @('LOCALAPPDATA','XDG_CONFIG_HOME','XDG_DATA_HOME','TEMP','TMP')) {
        $dir = Join-Path $Workspace $name
        New-Item -ItemType Directory -Path $dir | Out-Null
        [Environment]::SetEnvironmentVariable($name,$dir,'Process')
    }
    Invoke-Checked 'clone' 'git' @('clone','--branch',$status.branch,$repoUrl,$source)
    Invoke-Checked 'checkout' 'git' @('-C',$source,'checkout','--detach',$Commit)
    $actual = & git -C $source rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $Commit) { throw 'Checkout SHA mismatch' }
    Invoke-Checked 'dependencies-python' 'python' @('-c','import numpy, PIL; print(numpy.__version__, PIL.__version__)')
    Push-Location $source
    try {
        $lock = Get-Content 'tools/windows-bootstrap.lock.json' -Raw | ConvertFrom-Json
        if ((Get-FileHash -LiteralPath $Rpx -Algorithm SHA256).Hash -ne $lock.rpx_sha256) { throw 'RPX hash differs from the supported source image.' }
        Invoke-Checked 'python-tests' 'python' @('tools/test_character_importer.py')
        & tools/setup_windows_runtime.ps1 -ToolsRoot $ToolsRoot *> (Join-Path $reports 'setup.log')
        & tools/build_windows_tools.ps1 -ToolsRoot $ToolsRoot *> (Join-Path $reports 'tools.log')
        Invoke-Checked 'recompile' (Join-Path $source 'build-windows-tools/recomp/cafe-recomp.exe') @($Rpx,(Join-Path $source 'generated/ttt2-windows'))
        & tools/build_windows_runtime.ps1 -ToolsRoot $ToolsRoot -Rpx $Rpx *> (Join-Path $reports 'runtime.log')
        $status.build = 'passed'
        $inputs = @($Rpx) + @(Get-ChildItem -LiteralPath $Movesets -File -Filter '*.tkmvst' | ForEach-Object FullName)
        @($inputs | ForEach-Object { Get-FileHash -LiteralPath $_ -Algorithm SHA256 }) | ConvertTo-Json | Set-Content (Join-Path $reports 'input-hashes.json')
        foreach ($pass in @('a','b')) {
            $pack = Join-Path $Workspace "characters-$pass"
            Invoke-Checked "import-$pass" 'python' @('-u','tools/characters/import_character.py','akuma','--game',$Game,'--tekken7',$Tekken7,'--movesets',$Movesets,'--out',$pack,'--outfits','none')
            Invoke-Checked "validate-$pass" 'python' @('tools/characters/tests/verify_akuma_alpha_pack.py',(Join-Path $pack 'mrx/mrx.moveset'),'--report',(Join-Path $reports "pack-$pass.json"))
            Write-Manifest $pack (Join-Path $reports "manifest-$pass.json")
        }
        $status.import = 'passed'
        if ((Get-Content (Join-Path $reports 'manifest-a.json') -Raw) -ne (Get-Content (Join-Path $reports 'manifest-b.json') -Raw)) {
            throw 'Fresh character packs differ; inspect manifests before gameplay.'
        }
        $status.deterministic = 'passed'
        Write-Manifest (Join-Path $source 'build-windows-tools') (Join-Path $reports 'build-manifest.json')
        $save = Join-Path $Workspace 'save'
        New-Item -ItemType Directory -Path $save | Out-Null
        if ($SaveSeed) { Copy-Item -Path (Join-Path $SaveSeed '*') -Destination $save -Recurse }
        Copy-Item -LiteralPath $save -Destination (Join-Path $Workspace 'save-backup') -Recurse
        # Rollback never touches the reference installation, and retains test saves.
        @'
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($PSScriptRoot)
$save=Join-Path $root 'save'
if (Get-Process ttt2-ultimate -ErrorAction SilentlyContinue) { throw 'Close the game before rolling back.' }
if (!(Test-Path -LiteralPath (Join-Path $root 'save-backup'))) { throw 'Backup missing.' }
if (Test-Path -LiteralPath $save) { Move-Item -LiteralPath $save -Destination (Join-Path $root ('save-before-rollback-'+[Guid]::NewGuid().ToString('N'))) }
Copy-Item -LiteralPath (Join-Path $root 'save-backup') -Destination $save -Recurse
'@ | Set-Content -LiteralPath (Join-Path $Workspace 'Rollback.ps1') -Encoding UTF8
        if ($Launch) {
            $env:TTT2_CHARACTERS = Join-Path $Workspace 'characters-a'
            $env:TTT2_SAVE_DIR = $save
            $env:TTT2_TRACE_AKUMA_CONTACT = '1'
            if ($Experiments) {
                foreach ($name in @('TTT2_TEST_AKUMA_EX_RUNTIME','TTT2_TEST_AKUMA_EX_STAGE2','TTT2_TEST_AKUMA_SUPER_RUNTIME','TTT2_HITSTOP_LP_HEAVY','TTT2_HITSTOP_SPECIALS','TTT2_TEST_DFLIP_FOLLOWUP_FIX','TTT2_TEST_DFLIP_ROOT_HEIGHT','TTT2_TEST_DFLIP_TAKEOFF_SYNC','TTT2_TEST_AKUMA_NEUTRAL_METER')) {
                    [Environment]::SetEnvironmentVariable($name,'1','Process')
                }
            }
            $runtimeGame = if (Test-Path -LiteralPath (Join-Path $Game 'game')) { Join-Path $Game 'game' } else { $Game }
            Invoke-Checked 'game' (Join-Path $source 'build-windows-tools/ttt2-ultimate.exe') @($runtimeGame)
            $status.gameplay = 'launched; manual acceptance and measured parity remain required'
        }
    } finally { Pop-Location }
} catch {
    $status.error = $_.Exception.Message
    throw
} finally {
    $status | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $reports 'status.json') -Encoding UTF8
    'Record six jump routes, EX 2 contacts/9 ticks/8+5 stop, Super 5 contacts/7 ticks/6+6+6+6+5 stop, Demon Flip, hit/block cancels, meter off/on, tags, Tag Assault, other fighters and projectiles. Include measured results; launch alone is not parity.' | Set-Content (Join-Path $reports 'GAMEPLAY-PENDING.txt')
    foreach ($name in @(Get-ChildItem Env: | Where-Object Name -Like 'TTT2_*' | ForEach-Object Name)) { [Environment]::SetEnvironmentVariable($name,$null,'Process') }
    foreach ($name in $savedEnv.Keys) { [Environment]::SetEnvironmentVariable($name,$savedEnv[$name],'Process') }
    # Explicit reports directory only: no generated code, assets, packs, or executables.
    Compress-Archive -LiteralPath $reports.FullName -DestinationPath (Join-Path $Workspace 'feedback-reports.zip')
    Write-Host "Evidence: $Workspace\feedback-reports.zip"
}
