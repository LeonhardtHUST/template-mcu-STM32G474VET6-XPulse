#!/usr/bin/env bash
#
# 禁用 clangd：撤掉 clangd / cpptools 相关配置，把 IntelliSense 交回 PlatformIO。
#
# 用法：
#   ./scripts/disable-clangd.sh
#
# 说明：
#   - 只改本机文件，不动版本库。
#   - .vscode/settings.json 删除 C_Cpp.* 与 clangd.* 后备份为 settings.json.bak。
#   - .clangd 直接删除；compile_commands.json 保留，恢复时不必再生成。

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SETTINGS='.vscode/settings.json'

echo "==> [1/2] 清理 $SETTINGS"
if [ ! -f "$SETTINGS" ]; then
  echo "    未找到 $SETTINGS，跳过"
else
  TMP="$SETTINGS.tmp"
  awk '
    /^[ \t]*"clangd\.arguments"[ \t]*:/ { in_arr = 1; next }
    in_arr { if ($0 ~ /^[ \t]*\][ \t]*,?[ \t]*$/) in_arr = 0; next }
    /^[ \t]*\/\/.*(clangd|cpptools)/ { next }
    /^[ \t]*"(clangd|C_Cpp)\./ { next }
    { buf[++n] = $0 }
    END {
      for (i = 1; i <= n; i++) {
        if (buf[i] ~ /,[ \t]*$/ && buf[i + 1] ~ /^[ \t]*}[ \t]*$/) sub(/,[ \t]*$/, "", buf[i])
        print buf[i]
      }
    }
  ' "$SETTINGS" > "$TMP"

  if cmp -s "$SETTINGS" "$TMP"; then
    echo "    没有需要移除的配置"
    rm -f "$TMP"
  else
    if [ "$(grep -o '{' "$TMP" | wc -l)" -ne "$(grep -o '}' "$TMP" | wc -l)" ]; then
      rm -f "$TMP"
      echo "错误：改写后花括号不匹配，已放弃。请手动编辑 $SETTINGS" >&2
      exit 1
    fi
    cp "$SETTINGS" "$SETTINGS.bak"
    mv "$TMP" "$SETTINGS"
    echo "    已移除 clangd / C_Cpp 配置（备份：$SETTINGS.bak）"
  fi
fi

echo "==> [2/2] 删除 .clangd"
if [ -f .clangd ]; then
  rm -f .clangd
  echo "    已删除 .clangd（compile_commands.json 保留）"
else
  echo "    未找到 .clangd，跳过"
fi

echo
echo "完成。还需在扩展面板把 clangd 设为 Disable (Workspace)，PlatformIO 会重建 c_cpp_properties.json。"
echo "恢复：./scripts/init-clangd.sh"
