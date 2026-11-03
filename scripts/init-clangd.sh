#!/usr/bin/env bash
#
# 初始化 clangd 语义索引：生成 compile_commands.json、.clangd 与 VS Code settings。
#
# 用法：
#   ./scripts/init-clangd.sh
#
# 说明：
#   - 不使用 heredoc，全部配置用逐行 printf 写出，便于阅读和按行修改。
#   - .vscode/settings.json 已存在时不会被覆盖，避免冲掉原有的 files.exclude 等配置。

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

if ! command -v pio >/dev/null 2>&1; then
  echo "错误：找不到 pio，请先安装 PlatformIO Core（pip install platformio）" >&2
  exit 1
fi

echo "==> [1/3] 生成 compile_commands.json"
pio run -t compiledb

# .clangd 让 clangd 在本项目使用根目录的编译数据库
echo "==> [2/3] 写入 .clangd"
printf '%s\n' \
  'CompileFlags:' \
  '  CompilationDatabase: .' \
  > .clangd

echo "==> [3/3] 检查 .vscode/settings.json"
mkdir -p .vscode
SETTINGS='.vscode/settings.json'

if [ ! -f "$SETTINGS" ]; then
  # 与仓库自带的 .vscode/settings.json 保持一致：既配 clangd，也隐藏 .pio
  printf '%s\n' \
    '{' \
    '  "files.exclude": {' \
    '    "**/.pio": true' \
    '  },' \
    '  "search.exclude": {' \
    '    "**/.pio": true' \
    '  },' \
    '  "files.associations": {' \
    '    "*.ioc": "ini"' \
    '  },' \
    '  "C_Cpp.intelliSenseEngine": "disabled",' \
    '  "clangd.arguments": [' \
    '    "--compile-commands-dir=${workspaceFolder}",' \
    '    "--query-driver=**/arm-none-eabi-gcc,**/arm-none-eabi-g++,**/arm-none-eabi-gcc.exe,**/arm-none-eabi-g++.exe",' \
    '    "--background-index",' \
    '    "--clang-tidy"' \
    '  ],' \
    '  "clangd.restartAfterCrash": true' \
    '}' \
    > "$SETTINGS"
  echo "    已创建 $SETTINGS"
elif grep -q '"clangd.arguments"' "$SETTINGS"; then
  echo "    $SETTINGS 已含 clangd 配置，保持不变"
else
  echo "    警告：$SETTINGS 已存在但缺少 clangd.arguments，请手动合并（见 README 的 clangd 初始化）" >&2
fi

echo
echo "完成。请在 VS Code 命令面板执行：clangd: Restart language server"
