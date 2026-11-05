// 提交信息校验：在 @commitlint/config-conventional 之上补齐 AGENTS.md §1 的中文规范。
//
// 校验边界（重要）：
//   - 机器能判定的：简述含汉字（至少一个）/ ≤50 字 / 结尾无句号。
//   - 机器判不准的：「动词开头」属于语义判断，中文动词没有可靠词表，
//     强行正则会大量误伤，因此放在 AGENTS.md §1.1 的人工核对清单里。
//   - 验证等级、构建数字由 AGENTS.md §1.1 / §3 要求人工如实填写，本文件不强制。
//
// 规则返回 [是否通过, 失败提示]（commitlint plugin rule 约定）。

const SUBJECT_MAX_CHARS = 50;

const chineseSubject = (parsed) => {
  const subject = parsed.subject || '';
  // 只要求「含汉字」，不限定首字：允许 `docs(ioc): Q4 决策行回指 Q15` 这类
  // 以型号 / 编号 / 英文标识开头的简述。全英文或全符号仍然拒绝。
  if (!/[\u4e00-\u9fa5]/.test(subject)) {
    return [false, '简述必须使用中文（至少含一个汉字）'];
  }
  const length = Array.from(subject).length;
  if (length > SUBJECT_MAX_CHARS) {
    return [false, `简述不得超过 ${SUBJECT_MAX_CHARS} 字（当前 ${length} 字）`];
  }
  if (/[。．.；;]$/.test(subject)) {
    return [false, '简述结尾不得加句号或分号'];
  }
  return [true, ''];
};

export default {
  extends: ['@commitlint/config-conventional'],
  plugins: [
    {
      rules: {
        'chinese-subject': chineseSubject,
      },
    },
  ],
  rules: {
    // config-conventional 的 subject-case 面向英文大小写，会把中文与
    // 「Q4」「GPIO」这类混排简述判成 sentence-case；汉字与长度已由
    // chinese-subject 覆盖，关掉以避免误伤。
    'subject-case': [0],
    'chinese-subject': [2, 'always'],
  },
};
