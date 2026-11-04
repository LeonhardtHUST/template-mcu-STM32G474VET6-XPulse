#!/usr/bin/env node
/**
 * 项目初始化：重命名 .ioc，并同步 .ioc 内的项目名与仓库里的引用。
 *
 * 用法：
 *   node scripts/init-project.mjs                     交互式
 *   node scripts/init-project.mjs my-board            直接指定项目名
 *   node scripts/init-project.mjs my-board --yes      跳过确认
 *   node scripts/init-project.mjs my-board --dry-run  只打印计划
 */

import { readFile, writeFile, rename, readdir, access } from 'node:fs/promises';
import { createInterface } from 'node:readline/promises';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const argv = process.argv.slice(2);
const dryRun = argv.includes('--dry-run');
const skipConfirm = argv.includes('--yes') || argv.includes('-y');
const nameArg = argv.find((arg) => !arg.startsWith('-'));

const iocFiles = (await readdir(repoRoot)).filter((file) => file.endsWith('.ioc'));
if (iocFiles.length === 0) die('仓库根目录没有 .ioc 文件');
if (iocFiles.length > 1) die(`根目录有多个 .ioc（${iocFiles.join(', ')}），请只保留一个`);

const [iocFile] = iocFiles;

const oldName = iocFile.replace(/\.ioc$/, '');
const oldSlug = slugify(oldName);

let newName = nameArg;
if (!newName) {
    if (!process.stdin.isTTY) die('非交互环境请直接给出项目名，如：vpr init my-board --yes');
    const rl = createInterface({ input: process.stdin, output: process.stdout });
    newName = (await rl.question(`项目名（当前 ${oldName}）：`)).trim();
    rl.close();
}

if (!/^[A-Za-z0-9][A-Za-z0-9._-]*$/.test(newName)) {
    die('项目名只能包含字母、数字和 . _ -，且以字母或数字开头');
}
if (newName === oldName) {
    console.log(`项目名已是 ${newName}，无需改动。`);
    process.exit(0);
}
if (await exists(join(repoRoot, `${newName}.ioc`))) {
    die(`${newName}.ioc 已存在，请换一个名字`);
}
if (newName.length > 40) {
    console.warn(`警告：${newName} 有 ${newName.length} 个字符，Windows 深路径容易撞 MAX_PATH。`);
}

const newSlug = slugify(newName);
const oldIocPath = join(repoRoot, iocFile);
const newIocPath = join(repoRoot, `${newName}.ioc`);

// 需要跟着改名的引用文件
const targets = [
    { path: join(repoRoot, 'README.md'), label: 'README.md', mode: 'replace' },
    { path: join(repoRoot, '.vscode', 'tasks.json'), label: '.vscode/tasks.json', mode: 'replace', optional: true },
    { path: join(repoRoot, 'package.json'), label: 'package.json', mode: 'slug', optional: true },
];

const plan = [{ action: 'rename', from: iocFile, to: `${newName}.ioc` }];
const iocText = await readFile(oldIocPath, 'utf8');
if (!iocText.includes(oldName)) {
    die(`${iocFile} 里找不到项目名 ${oldName}，可能已被手工改过`);
}
plan.push({ action: 'patch', label: iocFile, hits: count(iocText, oldName) });

const writes = [];
for (const target of targets) {
    if (!(await exists(target.path))) {
        if (!target.optional) die(`找不到 ${target.label}`);
        continue;
    }
    const text = await readFile(target.path, 'utf8');
    const next = target.mode === 'slug' ? text.replace(`"name": "${oldSlug}"`, `"name": "${newSlug}"`) : replaceAll(text, oldName, newName);
    if (next === text) {
        if (target.mode === 'slug') continue;
        continue;
    }
    const hits = count(text, target.mode === 'slug' ? oldSlug : oldName);
    plan.push({ action: 'patch', label: target.label, hits });
    writes.push({ path: target.path, text: next });
}

console.log(`项目名：${oldName} → ${newName}`);
for (const item of plan) {
    if (item.action === 'rename') console.log(`  重命名 ${item.from} → ${item.to}`);
    else console.log(`  更新 ${item.label}（${item.hits} 处）`);
}
if (!writes.length) console.log('  （没有需要同步的引用）');

if (dryRun) {
    console.log('--dry-run：未写入任何文件。');
    process.exit(0);
}

if (!skipConfirm) {
    if (!process.stdin.isTTY) die('非交互环境请加 --yes 跳过确认');
    const rl = createInterface({ input: process.stdin, output: process.stdout });
    const answer = (await rl.question('确认执行？[y/N] ')).trim().toLowerCase();
    rl.close();
    if (answer !== 'y' && answer !== 'yes') {
        console.log('已取消。');
        process.exit(0);
    }
}

await writeFile(oldIocPath, replaceAll(iocText, oldName, newName), 'utf8');
await rename(oldIocPath, newIocPath);
for (const write of writes) {
    await writeFile(write.path, write.text, 'utf8');
}

console.log('');
console.log('完成。还剩两件事脚本做不了：');
console.log('  1. 目录名仍是旧的，需要重命名就手动改（先关掉 VS Code 与 CubeMX）。');
console.log('  2. 若已配置 clangd，重新生成一次索引：vpr clangd');

function slugify(name) {
    return name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
}

function count(text, token) {
    return text.split(token).length - 1;
}

function replaceAll(text, from, to) {
    return text.split(from).join(to);
}

async function exists(path) {
    try {
        await access(path);
        return true;
    } catch {
        return false;
    }
}

function die(message) {
    console.error(`错误：${message}`);
    process.exit(1);
}
