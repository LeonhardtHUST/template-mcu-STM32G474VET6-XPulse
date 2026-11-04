#Requires -Version 5.1
<#
.SYNOPSIS
    禁用 clangd：撤掉 clangd / cpptools 相关配置，把 IntelliSense 交回 PlatformIO。

.DESCRIPTION
    只改本机文件，不动版本库：
      - .vscode/settings.json：删除 C_Cpp.* 静音项与 clangd.* 配置，原文件备份为 settings.json.bak
      - .clangd：删除（compile_commands.json 保留，重新启用时不必再生成）

    脚本未签名，需要绕过执行策略：
        powershell -ExecutionPolicy Bypass -File .\scripts\disable-clangd.ps1

.EXAMPLE
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\scripts\disable-clangd.ps1
#>

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $repoRoot

$settingsPath = Join-Path $repoRoot '.vscode/settings.json'
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

Write-Host '==> [1/2] 清理 .vscode/settings.json'
if (-not (Test-Path $settingsPath)) {
    Write-Host '    未找到 settings.json，跳过'
}
else {
    $original = [System.IO.File]::ReadAllText($settingsPath)
    $text = $original

    # clangd.arguments 是数组，整块删除
    $text = [regex]::Replace($text, '(?ms)^[ \t]*"clangd\.arguments"[ \t]*:[ \t]*\[.*?\][ \t]*,?[ \t]*\r?\n', '')
    # 单行键
    $text = [regex]::Replace($text, '(?m)^[ \t]*"(?:clangd|C_Cpp)\.[^"\r\n]*"[^\r\n]*(?:\r?\n)', '')
    # 解释这些配置的注释行
    $text = [regex]::Replace($text, '(?m)^[ \t]*//[^\r\n]*(?:clangd|cpptools)[^\r\n]*(?:\r?\n)', '')
    # 删完最后一项后可能留下悬空逗号
    $text = [regex]::Replace($text, ',[ \t]*(\r?\n[ \t]*})', '$1')

    if ($text -eq $original) {
        Write-Host '    没有需要移除的配置'
    }
    else {
        $open = [regex]::Matches($text, '\{').Count
        $close = [regex]::Matches($text, '\}').Count
        $openArr = [regex]::Matches($text, '\[').Count
        $closeArr = [regex]::Matches($text, '\]').Count
        if ($open -ne $close -or $openArr -ne $closeArr) {
            throw '改写后括号不匹配，已放弃。请手动编辑 .vscode/settings.json。'
        }
        Copy-Item $settingsPath "$settingsPath.bak" -Force
        [System.IO.File]::WriteAllText($settingsPath, $text, $utf8NoBom)
        Write-Host '    已移除 clangd / C_Cpp 配置（备份：.vscode/settings.json.bak）'
    }
}

Write-Host '==> [2/2] 删除 .clangd'
if (Test-Path (Join-Path $repoRoot '.clangd')) {
    Remove-Item (Join-Path $repoRoot '.clangd') -Force
    Write-Host '    已删除 .clangd（compile_commands.json 保留）'
}
else {
    Write-Host '    未找到 .clangd，跳过'
}

Write-Host ''
Write-Host '完成。还需在扩展面板把 clangd 设为 Disable (Workspace)，PlatformIO 会重建 c_cpp_properties.json。'
Write-Host '恢复：.\scripts\init-clangd.ps1'
