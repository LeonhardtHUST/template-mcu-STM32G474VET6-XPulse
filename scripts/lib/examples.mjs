/**
 * examples/ 目录删除与 README 引用清理的共享实现。
 *
 * `scripts/init-project.mjs`（初始化流程内）与 `scripts/remove-examples.mjs`
 * （独立一键删除入口）都调用本模块，保证两份入口的删除行为不会漂移。
 *
 * 安全约定：
 *   - 删除目标恒为仓库根目录下的一级 `examples/` 目录，绝不接受外部传入的路径；
 *   - 执行前校验解析后的路径仍在 repoRoot 内、真实路径也在 repoRoot 内、
 *     目标确为目录且不是符号链接；
 *   - `docs/XPulse/例程/` 等其它目录下的任何文件都不在本模块的访问范围内；
 *   - 删除只能用 fs.rm(path, { recursive: true, force: true })。
 *
 * 原子性约定（P1 根因修复）：
 *   - 任何删除动作之前先做**预检**：读取 README、校验 BEGIN/END 标记成对且唯一、
 *     计算清理后的文本、确认 README 可写；预检失败一律抛出、零副作用；
 *   - 预检与 --dry-run 走同一条路径，预检失败时 --dry-run 同样拒绝；
 *   - 只有预检通过后才能 rm；rm 之后写 README 失败时，用内存中的原文尽力回滚
 *     并抛出明确错误，不留下静默的半完成状态。
 */

import { access, lstat, readFile, realpath, rm, writeFile } from 'node:fs/promises';
import { constants } from 'node:fs';
import { basename, isAbsolute, join, relative, resolve, sep } from 'node:path';

/** README 中 examples 引用区块的起始标记。 */
export const EXAMPLES_BEGIN = '<!-- BEGIN EXAMPLES -->';
/** README 中 examples 引用区块的结束标记。 */
export const EXAMPLES_END = '<!-- END EXAMPLES -->';

const README_NAME = 'README.md';

/**
 * 计算并强校验 examples 目录的绝对路径。
 *
 * @param {string} repoRoot 仓库根目录绝对路径。
 * @returns {string} `repoRoot/examples` 的规范化绝对路径。
 * @throws {Error} 目标不是仓库内的一级 examples 目录时抛出。
 */
export function resolveExamplesDir(repoRoot) {
  const root = resolve(repoRoot);
  const dir = resolve(root, 'examples');
  const rel = relative(root, dir).split(sep).join('/');
  if (rel !== 'examples' || isAbsolute(rel) || rel.startsWith('..')) {
    throw new Error(`拒绝操作：目标路径越出仓库根目录（${dir}）`);
  }
  if (basename(dir) !== 'examples') {
    throw new Error(`拒绝操作：目标不是 examples 目录（${dir}）`);
  }
  return dir;
}

/**
 * 对目标做 lstat；不存在时返回 null，不视为错误。
 *
 * @param {string} path 待探测路径。
 * @returns {Promise<import('node:fs').Stats | null>} 探测结果。
 */
async function lstatOrNull(path) {
  try {
    return await lstat(path);
  } catch {
    return null;
  }
}

/**
 * 检查 examples 目录是否存在、是否为目录、真实路径是否仍在仓库内。
 *
 * @param {string} repoRoot 仓库根目录绝对路径。
 * @returns {Promise<{repoRoot: string, examplesDir: string, exists: boolean}>} 检查结果。
 * @throws {Error} 目标是符号链接、非目录或真实路径越界时抛出。
 */
export async function inspectExamples(repoRoot) {
  const root = resolve(repoRoot);
  const dir = resolveExamplesDir(root);
  const entry = await lstatOrNull(dir);
  if (entry === null) {
    return { repoRoot: root, examplesDir: dir, exists: false };
  }
  if (entry.isSymbolicLink()) {
    throw new Error('拒绝删除：examples/ 是符号链接，请人工确认后再处理');
  }
  if (!entry.isDirectory()) {
    throw new Error('拒绝删除：examples/ 不是目录');
  }

  // realpath 校验：防止目录被挂载点/联接点替换到仓库之外。
  const [realRoot, realDir] = await Promise.all([realpath(root), realpath(dir)]);
  const realRel = relative(realRoot, realDir).split(sep).join('/');
  if (realRel === '' || realRel.startsWith('..') || isAbsolute(realRel)) {
    throw new Error(`拒绝删除：examples/ 的真实路径越出仓库根目录（${realDir}）`);
  }
  if (realRel !== 'examples') {
    throw new Error(`拒绝删除：examples/ 的真实路径不是仓库内的一级 examples/（${realRel}）`);
  }
  return { repoRoot: root, examplesDir: dir, exists: true };
}

/**
 * 从 README 文本中移除 EXAMPLES 标记区块（含两行标记本身）。
 *
 * 采用逐行匹配，兼容 LF 与 CRLF。BEGIN / END 必须**各出现且仅出现一次**且顺序正确；
 * 缺标记、重复标记、顺序颠倒统统抛错而不是猜测，避免破坏文档或留下死链。
 * 两者都不存在时视为「无需清理」，保证删除操作可重复执行。
 *
 * @param {string} text README 原文。
 * @returns {{changed: boolean, text: string}} 是否变更与结果文本。
 * @throws {Error} 标记缺失、重复或顺序错误时抛出。
 */
export function stripExamplesBlock(text) {
  const eol = text.includes('\r\n') ? '\r\n' : '\n';
  const lines = text.split(/\r?\n/);
  const beginIndices = [];
  const endIndices = [];
  lines.forEach((line, index) => {
    const trimmed = line.trim();
    if (trimmed === EXAMPLES_BEGIN) beginIndices.push(index);
    if (trimmed === EXAMPLES_END) endIndices.push(index);
  });

  if (beginIndices.length === 0 && endIndices.length === 0) {
    return { changed: false, text };
  }
  if (beginIndices.length !== 1 || endIndices.length !== 1) {
    throw new Error(
      `${README_NAME} 的 EXAMPLES 标记必须各出现一次：` +
      `BEGIN ${beginIndices.length} 次、END ${endIndices.length} 次；拒绝清理以免破坏文档`
    );
  }
  const begin = beginIndices[0];
  const end = endIndices[0];
  if (end < begin) {
    throw new Error(
      `${README_NAME} 的 END EXAMPLES 标记出现在 BEGIN 之前；拒绝清理以免破坏文档`
    );
  }

  let from = begin;
  let to = end;
  // 顺带吃掉紧邻的一个空行，避免清理后留下多余空白。
  if (to + 1 < lines.length && lines[to + 1].trim() === '') {
    to += 1;
  } else if (from > 0 && lines[from - 1].trim() === '') {
    from -= 1;
  }
  const next = [...lines.slice(0, from), ...lines.slice(to + 1)].join(eol);
  return { changed: next !== text, text: next };
}

/**
 * 删除前的统一预检（只读，绝不产生副作用）。
 *
 * 依次完成：examples 路径安全校验 → 读取 README（仅 ENOENT 容错）→ 校验 BEGIN/END
 * 标记成对且唯一 → 计算清理后的 README 文本 → 确认 README 可写。
 * 任何一步失败都抛出清晰错误，调用方据此在 rm 之前中止，保证零副作用。
 *
 * @param {string} repoRoot 仓库根目录绝对路径。
 * @returns {Promise<object>} 预检结果，同时可作为删除计划使用。
 * @throws {Error} 路径不安全、README 不可读/不可写或标记结构错误时抛出。
 */
async function preflightExamplesRemoval(repoRoot) {
  const info = await inspectExamples(repoRoot);
  const readmePath = join(info.repoRoot, README_NAME);
  /** @type {string | null} README 原文，ENOENT 时为 null。 */
  let readmeText = null;
  /** @type {string | null} 清理后的 README 文本。 */
  let readmeNext = null;
  /** @type {boolean} 是否需要写回 README。 */
  let readmeChanged = false;

  try {
    readmeText = await readFile(readmePath, 'utf8');
  } catch (error) {
    // 只容忍「README 不存在」；EACCES、EISDIR 等一律不得吞掉。
    if (error instanceof Error && error.code === 'ENOENT') {
      readmeText = null;
    } else {
      const reason = error instanceof Error ? error.message : String(error);
      throw new Error(
        `读取 ${README_NAME} 失败（${reason}）；预检未通过，未删除 examples/、未修改任何文件`
      );
    }
  }

  if (readmeText !== null) {
    // 预检可写性：把「删完目录才发现 README 写不进去」的风险拦在 rm 之前。
    try {
      await access(readmePath, constants.W_OK);
    } catch (error) {
      const reason = error instanceof Error ? error.message : String(error);
      throw new Error(
        `${README_NAME} 不可写（${reason}）；预检未通过，未删除 examples/、未修改任何文件`
      );
    }

    /** @type {{changed: boolean, text: string}} */
    let result;
    try {
      result = stripExamplesBlock(readmeText);
    } catch (error) {
      const reason = error instanceof Error ? error.message : String(error);
      throw new Error(`${reason}；预检未通过，未删除 examples/、未修改任何文件`);
    }
    readmeNext = result.text;
    readmeChanged = result.changed;
  }

  const lines = [];
  if (info.exists) {
    lines.push('将删除 examples/ 目录（含 INDEX.md、README.md、recipes/、stdio_port/）');
  } else {
    lines.push('examples/ 目录不存在，跳过删除');
  }
  if (readmeChanged) {
    lines.push(`将清理 ${README_NAME} 中的 EXAMPLES HTML 标记区块`);
  } else if (readmeText !== null) {
    lines.push(`${README_NAME} 中没有 EXAMPLES 标记区块，跳过引用清理`);
  }

  return {
    repoRoot: info.repoRoot,
    examplesDir: info.examplesDir,
    examplesExists: info.exists,
    readmePath,
    readmeText,
    readmeNext,
    readmeChanged,
    lines,
  };
}

/**
 * 生成 examples 删除计划（只读，供计划输出与 --dry-run 使用）。
 *
 * 它走的是与真正执行**完全相同**的预检，因此 --dry-run 能提前暴露标记结构错误，
 * 且不会写文件、不会删目录。
 *
 * @param {string} repoRoot 仓库根目录绝对路径。
 * @returns {Promise<object>} 删除计划对象，交给 applyExamplesRemoval 执行。
 * @throws {Error} 预检失败时抛出。
 */
export async function planExamplesRemoval(repoRoot) {
  return preflightExamplesRemoval(repoRoot);
}

/**
 * 执行 examples 删除：rm 之后写 README，写失败时用内存原文尽力回滚。
 *
 * 执行前会**重新**跑一次预检（初始化流程可能在计划生成后改写了 README），
 * 预检不通过则在 rm 之前中止，零副作用。路径安全校验也在此二次执行。
 *
 * @param {object} plan planExamplesRemoval 返回的计划对象。
 * @returns {Promise<{examplesRemoved: boolean, readmeCleaned: boolean}>} 执行结果。
 * @throws {Error} 预检失败、删除失败或 README 写入失败时抛出。
 */
export async function applyExamplesRemoval(plan) {
  const prep = await preflightExamplesRemoval(plan.repoRoot);
  let examplesRemoved = false;
  let readmeCleaned = false;

  if (prep.examplesExists) {
    await rm(prep.examplesDir, { recursive: true, force: true });
    examplesRemoved = true;
  }

  if (prep.readmeChanged && prep.readmeText !== null && prep.readmeNext !== null) {
    try {
      await writeFile(prep.readmePath, prep.readmeNext, 'utf8');
      readmeCleaned = true;
    } catch (error) {
      // 用内存中的原文尽力恢复，避免 README 处于半写入的静默损坏状态。
      let restored = false;
      try {
        await writeFile(prep.readmePath, prep.readmeText, 'utf8');
        restored = true;
      } catch {
        restored = false;
      }
      const reason = error instanceof Error ? error.message : String(error);
      throw new Error(
        `删除 examples/ 后写入 ${README_NAME} 失败（${reason}）；` +
        (restored
          ? `已用内存原文恢复 ${README_NAME}，请检查后重试`
          : `${README_NAME} 恢复失败，请人工检查并恢复文档`)
      );
    }
  }

  return { examplesRemoved, readmeCleaned };
}
