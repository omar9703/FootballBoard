param([string]$UnrealEditor = 'F:\unreal\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe')
$projectPath = Join-Path $PSScriptRoot 'FootballBoard.uproject'
if (!(Test-Path -LiteralPath $UnrealEditor)) { throw 'Indica la ruta de UnrealEditor.exe con -UnrealEditor.' }
foreach ($role in @('Sender', 'Receiver')) {
    $windowX = if ($role -eq 'Sender') { 20 } else { 1160 }
    $launchArguments = '"{0}" /Game/FootballBoard/Level_{1} -game -windowed -ResX=1100 -ResY=700 -WinX={2} -WinY=80 -ExecCmds="t.IdleWhenNotForeground 0,t.MaxFPS 60" -log=Board_{1}.log' -f $projectPath, $role, $windowX
    Start-Process -FilePath $UnrealEditor -ArgumentList $launchArguments
}
