#Requires -Version 5.1
<#
.SYNOPSIS
    初始化 clangd 语义索引：生成 compile_commands.json、.clangd 与 VS Code settings。

.DESCRIPTION
    配置全部用逐行数组写出，不使用 heredoc，便于阅读和按行修改。
    .vscode/settings.json 已存在时不会被覆盖，避免冲掉原有的 files.exclude 等配置。

    脚本未签名，需要绕过执行策略：
        powershell -ExecutionPolicy Bypass -File .\scripts\init-clangd.ps1

.EXAMPLE
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\scripts\init-clangd.ps1
#>

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $repoRoot

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw '找不到 pio，请先安装 PlatformIO Core（pip install platformio）或 VS Code 的 PlatformIO IDE 插件。'
}

# clangd 的 YAML 解析器不吃 BOM，所以统一用 UTF-8 无 BOM 写文件
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

Write-Host '==> [1/3] 生成 compile_commands.json'
pio run -t compiledb
if ($LASTEXITCODE -ne 0) {
    throw "pio run -t compiledb 失败（退出码 $LASTEXITCODE）"
}

# .clangd 让 clangd 在本项目使用根目录的编译数据库
Write-Host '==> [2/3] 写入 .clangd'
$clangdLines = @(
    'CompileFlags:'
    '  CompilationDatabase: .'
)
$clangdText = ($clangdLines -join "`n") + "`n"
[System.IO.File]::WriteAllText((Join-Path $repoRoot '.clangd'), $clangdText, $utf8NoBom)

Write-Host '==> [3/3] 检查 .vscode/settings.json'
$settingsDir = Join-Path $repoRoot '.vscode'
$settingsPath = Join-Path $settingsDir 'settings.json'

if (-not (Test-Path $settingsPath)) {
    New-Item -ItemType Directory -Force -Path $settingsDir | Out-Null

    # 与仓库自带的 .vscode/settings.json 保持一致：既配 clangd，也隐藏 .pio
    $settings = [ordered]@{
        'files.exclude'            = [ordered]@{ '**/.pio' = $true }
        'search.exclude'           = [ordered]@{ '**/.pio' = $true }
        'files.associations'       = [ordered]@{ '*.ioc' = 'ini' }
        'C_Cpp.intelliSenseEngine' = 'disabled'
        'clangd.arguments'         = @(
            '--compile-commands-dir=${workspaceFolder}'
            '--query-driver=**/arm-none-eabi-gcc.exe,**/arm-none-eabi-g++.exe'
            '--background-index'
            '--clang-tidy'
        )
        'clangd.restartAfterCrash' = $true
    }
    $json = ($settings | ConvertTo-Json -Depth 5) + "`n"
    [System.IO.File]::WriteAllText($settingsPath, $json, $utf8NoBom)
    Write-Host "    已创建 $settingsPath"
}
elseif (Select-String -Path $settingsPath -Pattern 'clangd.arguments' -Quiet) {
    Write-Host "    $settingsPath 已含 clangd 配置，保持不变"
}
else {
    Write-Warning "$settingsPath 已存在但缺少 clangd.arguments，请手动合并（见 README 的 clangd 初始化）。"
}

Write-Host ''
Write-Host '完成。请在 VS Code 命令面板执行：clangd: Restart language server'
