#!/usr/bin/env node
/**
 * 跨平台转发到 scripts/init-clangd.* 或 scripts/disable-clangd.*。
 *
 * 用法：
 *   node scripts/clangd.mjs            初始化（生成 compile_commands.json、.clangd、settings）
 *   node scripts/clangd.mjs disable    禁用 clangd，交回 PlatformIO IntelliSense
 */

import { spawnSync } from 'node:child_process';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const base = process.argv[2] === 'disable' ? 'disable-clangd' : 'init-clangd';
const win = process.platform === 'win32';

const command = win ? 'powershell' : 'bash';
const args = win
    ? ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', join('scripts', `${base}.ps1`)]
    : [join('scripts', `${base}.sh`)];

const { status } = spawnSync(command, args, { cwd: repoRoot, stdio: 'inherit' });
process.exit(status ?? 1);
