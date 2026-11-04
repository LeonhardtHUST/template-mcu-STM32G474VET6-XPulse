#!/usr/bin/env node
/**
 * 独立一键删除 examples/ 目录（无需改名、无需项目名）。
 *
 * 用法：
 *   node scripts/remove-examples.mjs             直接删除
 *   node scripts/remove-examples.mjs --dry-run   只打印计划，不写文件、不删目录
 *   node scripts/remove-examples.mjs --yes       兼容参数，行为与默认一致
 *
 * 命令本身即「显式删除意图」，因此不再追加二次确认；想先看后果请加 --dry-run。
 * 对应 npm/pnpm/vpr 别名：examples:remove。
 */

import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { applyExamplesRemoval, planExamplesRemoval } from './lib/examples.mjs';

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const argv = process.argv.slice(2);
const dryRun = argv.includes('--dry-run');
const knownFlags = new Set(['--dry-run', '--yes', '-y']);
const unknown = argv.filter((arg) => !knownFlags.has(arg));

if (unknown.length > 0) {
  die(`未知参数：${unknown.join(' ')}；可用参数：--dry-run、--yes`);
}

try {
  const plan = await planExamplesRemoval(repoRoot);
  console.log('examples/ 删除计划：');
  for (const line of plan.lines) {
    console.log(`  ${line}`);
  }
  console.log('  docs/XPulse/ 下的原始资料不在删除范围内。');

  if (dryRun) {
    console.log('--dry-run：未删除任何目录、未写入任何文件。');
    process.exit(0);
  }

  const result = await applyExamplesRemoval(plan);
  console.log('');
  console.log(result.examplesRemoved
    ? '完成：已删除 examples/（INDEX.md 随目录一起移除）。'
    : '完成：examples/ 本来就不存在，无需删除。');
  console.log(result.readmeCleaned
    ? '完成：已清理 README.md 中的 EXAMPLES 标记区块，无残留死链。'
    : '完成：README.md 无需清理 EXAMPLES 标记区块。');
  console.log('docs/XPulse/ 下的原始资料未改动。');
} catch (error) {
  die(error instanceof Error ? error.message : String(error));
}

/**
 * 打印错误并以退出码 1 结束。
 *
 * @param {string} message 错误说明。
 * @returns {never} 不返回。
 */
function die(message) {
  console.error(`错误：${message}`);
  process.exit(1);
}
