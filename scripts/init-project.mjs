#!/usr/bin/env node
/**
 * 项目初始化：重命名 .ioc、同步仓库里的项目名引用，并可按需删除 examples/。
 *
 * 用法：
 *   node scripts/init-project.mjs                      交互式（先问项目名，再问是否删示例）
 *   node scripts/init-project.mjs my-board             直接指定项目名
 *   node scripts/init-project.mjs my-board --yes       跳过所有交互询问
 *   node scripts/init-project.mjs my-board --dry-run   只打印计划，不写文件、不删目录
 *   node scripts/init-project.mjs --no-examples        只删 examples/，不改项目名
 *   node scripts/init-project.mjs --keep-examples      明确保留 examples/，不询问
 *
 * examples 相关约定：
 *   - 默认 N = 保留；交互时单独询问「是否删除 examples/ 目录？[y/N]」；
 *   - --no-examples / --keep-examples 互斥，显式给出时不再询问；
 *   - --yes 只跳过确认，未指定 examples 参数时按默认保留；
 *   - --dry-run 只显示计划（含「将删除 examples/」），不产生任何副作用；
 *   - 删除逻辑复用 scripts/lib/examples.mjs，与独立脚本行为一致。
 */

import { readFile, writeFile, rename, readdir, access } from 'node:fs/promises';
import { fstatSync, readFileSync } from 'node:fs';
import { createInterface } from 'node:readline/promises';
import { dirname, join, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { applyExamplesRemoval, planExamplesRemoval } from './lib/examples.mjs';

/**
 * 交互式提问器：TTY 用 readline；非 TTY 且 stdin 是管道/文件时按行取答案。
 * 这样既不会在真正的非交互环境里卡住，也能支持管道输入做回归测试。
 */
class Prompt {
  /**
   * @param {{avoidStdin?: boolean}} options 是否完全避免读取 stdin。
   */
  constructor({ avoidStdin = false } = {}) {
    /** @type {boolean} */
    this.isTTY = process.stdin.isTTY === true;
    /** @type {boolean} */
    this.avoidStdin = avoidStdin;
    /** @type {boolean} */
    this.loaded = false;
    /** @type {string[] | null} */
    this.lines = null;
    /** @type {number} */
    this.cursor = 0;
  }

  /**
   * 当前是否具备提问条件（TTY 或可读的管道输入）。
   *
   * @returns {boolean} 可以提问则为 true。
   */
  get canAsk() {
    if (this.isTTY) return true;
    if (this.avoidStdin) return false;
    return this.pipedLines() !== null;
  }

  /**
   * 惰性读取并缓存管道输入的各行。
   *
   * Windows 上匿名管道的 `Stats.isFIFO()` 可能返回 false（mode 仍是 S_IFIFO），
   * 因此不能用 isFIFO()/isFile() 白名单判断；只排除字符设备
   * （`/dev/null`、`CONIN$` 等），其余非 TTY 输入一律尝试按行读取。
   *
   * @returns {string[] | null} 行数组，无可用管道输入时为 null。
   */
  pipedLines() {
    if (this.loaded) return this.lines;
    this.loaded = true;
    if (this.avoidStdin) {
      this.lines = null;
      return null;
    }
    /** @type {import('node:fs').Stats} */
    let stat;
    try {
      stat = fstatSync(0);
    } catch {
      this.lines = null;
      return null;
    }
    if (stat.isCharacterDevice()) {
      this.lines = null;
      return null;
    }
    try {
      this.lines = readFileSync(0, 'utf8').split(/\r?\n/);
    } catch {
      this.lines = null;
    }
    return this.lines;
  }

  /**
   * 提出一个问题并返回去掉首尾空白的回答；无输入时返回空串（等价于默认 N）。
   *
   * @param {string} question 提示文本。
   * @returns {Promise<string>} 用户回答。
   */
  async ask(question) {
    if (this.isTTY) {
      const rl = createInterface({ input: process.stdin, output: process.stdout });
      const answer = (await rl.question(question)).trim();
      rl.close();
      return answer;
    }
    const lines = this.pipedLines();
    const answer = lines !== null && this.cursor < lines.length
      ? lines[this.cursor++].trim()
      : '';
    console.log(`${question}${answer}`);
    return answer;
  }
}

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const argv = process.argv.slice(2);
const dryRun = argv.includes('--dry-run');
const skipConfirm = argv.includes('--yes') || argv.includes('-y');
const noExamples = argv.includes('--no-examples');
const keepExamples = argv.includes('--keep-examples');
const hasExamplesFlag = noExamples || keepExamples;
const nameArg = argv.find((arg) => !arg.startsWith('-'));

if (noExamples && keepExamples) {
  die('--no-examples 与 --keep-examples 互斥，请只传其中一个');
}

// TTY 用 readline 逐问；非 TTY 且有管道/重定向输入时按行读取，保证管道测试可用。
// --yes / --dry-run 且非 TTY 时完全不碰 stdin，避免脚本在无输入的非交互环境里等待。
const prompt = new Prompt({
  avoidStdin: (skipConfirm || dryRun) && process.stdin.isTTY !== true,
});

const iocFiles = (await readdir(repoRoot)).filter((file) => file.endsWith('.ioc'));
if (iocFiles.length === 0) die('仓库根目录没有 .ioc 文件');
if (iocFiles.length > 1) die(`根目录有多个 .ioc（${iocFiles.join(', ')}），请只保留一个`);

const [iocFile] = iocFiles;
const oldName = iocFile.replace(/\.ioc$/, '');
const oldSlug = slugify(oldName);
const oldIocPath = join(repoRoot, iocFile);

/** @type {string} 目标项目名；不改名时等于 oldName。 */
let newName = oldName;
/** @type {boolean} 是否需要执行改名。 */
let renameNeeded = false;

if (nameArg) {
  validateName(nameArg);
  newName = nameArg;
  renameNeeded = newName !== oldName;
} else if (prompt.isTTY) {
  const answer = await prompt.ask(`项目名（当前 ${oldName}，留空保持不变）：`);
  if (answer) {
    validateName(answer);
    newName = answer;
    renameNeeded = newName !== oldName;
  }
} else {
  // 非交互且没给项目名：允许「只想删 examples、不改名」以及纯 --dry-run 的用法。
  // 既没有项目名、也没有明确的 examples 参数、且不是 dry-run 时，保留原有错误提示。
  if (!hasExamplesFlag && !dryRun && !prompt.canAsk) {
    die('非交互环境请直接给出项目名，如：vpr init my-board --yes');
  }
}

if (renameNeeded) {
  if (await exists(join(repoRoot, `${newName}.ioc`))) {
    die(`${newName}.ioc 已存在，请换一个名字`);
  }
}
if (renameNeeded && newName.length > 40) {
  console.warn(`警告：${newName} 有 ${newName.length} 个字符，Windows 深路径容易撞 MAX_PATH。`);
}

const newSlug = slugify(newName);
const newIocPath = join(repoRoot, `${newName}.ioc`);

/** @type {boolean} 是否删除 examples/。默认保留，交互或 CLI 可改变。 */
let removeExamples = noExamples;

// 需要跟着改名的引用文件；scripts/cubemx-generate.mx 硬编码了 .ioc 名，必须同步替换；
// examples/INDEX.md 纳入范围（内容不硬编码工程名，通常是空操作）。
const targets = [
  { path: join(repoRoot, 'README.md'), label: 'README.md', mode: 'replace' },
  { path: join(repoRoot, '.vscode', 'tasks.json'), label: '.vscode/tasks.json', mode: 'replace', optional: true },
  { path: join(repoRoot, 'scripts', 'cubemx-generate.mx'), label: 'scripts/cubemx-generate.mx', mode: 'replace', optional: true },
  { path: join(repoRoot, 'package.json'), label: 'package.json', mode: 'slug', optional: true },
  { path: join(repoRoot, 'examples', 'INDEX.md'), label: 'examples/INDEX.md', mode: 'replace', optional: true },
];

/** @type {Array<{action: string, from?: string, to?: string, label?: string, hits?: number, text?: string}>} */
const plan = [];
/** @type {Array<{path: string, text: string}>} */
const writes = [];
/** @type {string} 改名前的 .ioc 文本。 */
let iocText = '';

if (renameNeeded) {
  plan.push({ action: 'rename', from: iocFile, to: `${newName}.ioc` });
  iocText = await readFile(oldIocPath, 'utf8');
  if (!iocText.includes(oldName)) {
    die(`${iocFile} 里找不到项目名 ${oldName}，可能已被手工改过`);
  }
  plan.push({ action: 'patch', label: iocFile, hits: count(iocText, oldName) });

  for (const target of targets) {
    if (!(await exists(target.path))) {
      if (!target.optional) die(`找不到 ${target.label}`);
      continue;
    }
    const text = await readFile(target.path, 'utf8');
    const next = target.mode === 'slug'
      ? text.replace(`"name": "${oldSlug}"`, `"name": "${newSlug}"`)
      : replaceAll(text, oldName, newName);
    if (next === text) continue;
    const hits = count(text, target.mode === 'slug' ? oldSlug : oldName);
    plan.push({ action: 'patch', label: target.label, hits });
    writes.push({ path: target.path, text: next });
  }
} else {
  plan.push({ action: 'info', text: '项目名保持不变，跳过改名。' });
}

let examplesPlan = null;
/**
 * 惰性生成 examples 删除计划（只读），供计划输出与执行阶段共用。
 *
 * @returns {Promise<object>} 删除计划对象。
 */
async function getExamplesPlan() {
  if (examplesPlan === null) {
    try {
      examplesPlan = await planExamplesRemoval(repoRoot);
    } catch (error) {
      examplesError(error);
    }
  }
  return examplesPlan;
}

if (removeExamples) {
  const detail = await getExamplesPlan();
  for (const line of detail.lines) {
    plan.push({ action: 'examples', text: line });
  }
} else {
  plan.push({ action: 'examples', text: '保留 examples/（默认，不删除）' });
}

// ---- 计划输出 ----
console.log(renameNeeded ? `项目名：${oldName} → ${newName}` : `项目名保持 ${oldName}（不改名）`);
for (const item of plan) {
  if (item.action === 'rename') console.log(`  重命名 ${item.from} → ${item.to}`);
  else if (item.action === 'patch') console.log(`  更新 ${item.label}（${item.hits} 处）`);
  else console.log(`  ${item.text}`);
}
if (renameNeeded && writes.length === 0) console.log('  （没有需要同步的引用）');

if (dryRun) {
  console.log('--dry-run：未写入任何文件、未删除任何目录。');
  process.exit(0);
}

// ---- 单独询问 examples（在计划输出之后、总确认之前）----
if (!hasExamplesFlag && !skipConfirm && prompt.canAsk) {
  const answer = (await prompt.ask('是否删除 examples/ 目录？[y/N] ')).toLowerCase();
  if (answer === 'y' || answer === 'yes') {
    removeExamples = true;
    const detail = await getExamplesPlan();
    console.log('  → 已选择删除 examples/：');
    for (const line of detail.lines) console.log(`     ${line}`);
  } else {
    console.log('  → 保留 examples/（默认）。');
  }
}

if (!renameNeeded && !removeExamples) {
  console.log('没有需要执行的改动。');
  process.exit(0);
}

// ---- 总确认 ----
// --no-examples 本身即显式删除意图，单独使用时不再要一次总确认；
// 若同时要改名，仍保留原有的「确认执行？」以保证改名流程不回归。
const deletionConfirmedByCli = noExamples;
const needTotalConfirm = !skipConfirm
  && (renameNeeded || (removeExamples && !deletionConfirmedByCli));

if (needTotalConfirm) {
  if (!prompt.canAsk) die('非交互环境请加 --yes 跳过确认');
  const answer = (await prompt.ask('确认执行？[y/N] ')).toLowerCase();
  if (answer !== 'y' && answer !== 'yes') {
    console.log('已取消。');
    process.exit(0);
  }
}

// ---- 执行 ----
if (renameNeeded) {
  await writeFile(oldIocPath, replaceAll(iocText, oldName, newName), 'utf8');
  await rename(oldIocPath, newIocPath);
  const examplesPrefix = join(repoRoot, 'examples') + sep;
  const effectiveWrites = removeExamples
    ? writes.filter((item) => !item.path.startsWith(examplesPrefix))
    : writes;
  for (const item of effectiveWrites) {
    await writeFile(item.path, item.text, 'utf8');
  }
}

if (removeExamples) {
  /** @type {{examplesRemoved: boolean, readmeCleaned: boolean}} */
  let result;
  try {
    const detail = await getExamplesPlan();
    result = await applyExamplesRemoval(detail);
  } catch (error) {
    examplesError(error);
  }
  console.log('');
  console.log(result.examplesRemoved
    ? '已删除 examples/（INDEX.md 随目录一起移除）。'
    : 'examples/ 本来就不存在，无需删除。');
  console.log(result.readmeCleaned
    ? '已清理 README.md 中的 EXAMPLES 标记区块。'
    : 'README.md 无需清理 EXAMPLES 标记区块。');
  console.log('docs/XPulse/ 下的原始资料未改动。');
}

console.log('');
console.log('完成。还剩两件事脚本做不了：');
console.log('  1. 目录名仍是旧的，需要重命名就手动改（先关掉 VS Code 与 CubeMX）。');
console.log('  2. 若已配置 clangd，重新生成一次索引：vpr clangd');

/**
 * 校验项目名格式。
 *
 * @param {string} name 待校验的项目名。
 * @returns {void}
 */
function validateName(name) {
  if (!/^[A-Za-z0-9][A-Za-z0-9._-]*$/.test(name)) {
    die('项目名只能包含字母、数字和 . _ -，且以字母或数字开头');
  }
}

/**
 * 生成 npm 包名风格的 slug。
 *
 * @param {string} name 项目名。
 * @returns {string} slug。
 */
function slugify(name) {
  return name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
}

/**
 * 统计子串出现次数。
 *
 * @param {string} text 源文本。
 * @param {string} token 子串。
 * @returns {number} 出现次数。
 */
function count(text, token) {
  return text.split(token).length - 1;
}

/**
 * 全量替换子串（避免依赖 String.prototype.replaceAll 的转义语义）。
 *
 * @param {string} text 源文本。
 * @param {string} from 被替换内容。
 * @param {string} to 替换内容。
 * @returns {string} 替换结果。
 */
function replaceAll(text, from, to) {
  return text.split(from).join(to);
}

/**
 * 判断路径是否存在。
 *
 * @param {string} path 路径。
 * @returns {Promise<boolean>} 存在则为 true。
 */
async function exists(path) {
  try {
    await access(path);
    return true;
  } catch {
    return false;
  }
}

/**
 * examples 删除模块的统一错误出口。
 *
 * 与 `scripts/remove-examples.mjs` 的 die() 保持一致的呈现：单行中文错误、
 * 保留原始原因、不打印裸 Node 堆栈，退出码 1。
 *
 * @param {unknown} error 删除模块抛出的错误。
 * @returns {never} 不返回。
 */
function examplesError(error) {
  const reason = error instanceof Error ? error.message : String(error);
  die(`examples 操作失败：${reason}`);
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
