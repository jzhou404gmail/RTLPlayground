# 上游同步加固（pre/post sync guard）

## 为什么需要

`9122eb8 web: adopt the upstream single-page UI` 把 fork 的 SPA 换成上游版本时，
右上角的“退出登录”连同 `hdr_logout`、按钮和处理函数一起被覆盖掉了；密码改密的
随机 salt 也被换成了复用固件里的旧 salt。这类丢失不是编译错误，`make` 和主机
测试当时都不会报，只有上板或用户点按钮时才发现。

同步上游时有两类风险：

1. **工作区有未提交的改动**：merge/rebase 冲突时容易被 `checkout --theirs` 或
   `checkout origin/main -- <path>` 覆盖。
2. **本地提交只在本地**：`git reset --hard`、错误的 rebase 或换机器时丢失。

`tools/sync-guard.sh` 把这两件事变成同步流程里的强制检查。

## 同步前

```bash
git fetch origin
make pre-sync-check
```

它做这些事：

- 拒绝有未提交 tracked 改动的同步（先 commit 或 stash）；对未忽略的 untracked
  文件给出警告。
- 列出 `origin/main..HEAD` 的本地提交。
- 有本地提交时，自动创建 `pre-sync-<时间戳>` 标签，并在
  `.local-records/pre-sync-<时间戳>/` 写 `HEAD`、`BASE`、`format-patch` 补丁和
  `local.diff`。`--bundle` 还会额外写一个完整 git bundle（文件较大）。
- 检查 `HEAD` 是否已经在 `fork/main` 上；没有则提示先 push 到 fork 做异地备份。

如果确实要求“同步前一个本地提交都不能有”，用严格模式：

```bash
tools/sync-guard.sh pre --strict
```

严格模式发现有本地提交就直接失败，不创建备份；适合要求先 push/merge 的场景。

## 同步后

```bash
git merge origin/main        # 或 rebase
make post-sync-check
```

`post` 会检查这次修复所依赖的本地功能是否还在：

| 检查 | 目的 |
|---|---|
| `html/index.html` 的 `logoutBtn` | 右上角退出登录没有被 UI 覆盖 |
| `html/app.js` 的 `hdr_logout`、`$("logoutBtn")` | 文案和点击处理都在 |
| `html/app.js` 的 `crypto.getRandomValues(salt)` | 改密仍会轮换 salt |
| `html/app.js` 的 `passwd_hash`、`vlan switch` | 配置编辑器和 Save 合并仍认识本地命令 |
| `html/login.html` 的 `pw2row`、`legacy` | 首装设置页和旧明文回退都在 |
| `httpd/httpd.c` 的 `setup_body_take` | 首装/摘要 provisioning 还在 |
| `auth.c` 的 `auth_salt_tmp`、`"legacy"`、`No terminator` | 摘要解析保护和配置回写保护还在 |
| `rtlplayground.c` 的 `auth_persist` | 串口 `passwd_hash` 仍会落盘 |
| `tools/passwd_hash.py`、`test/test_auth.c`、`scenario_setup` | 工具和回归测试还在 |
| `test/test_web_auth.js` | 浏览器改密仍是 32 hex salt + 64 hex digest，`passwd_hash` 命令行不超长 |
| `node --check html/app.js` | 前端没有语法错误 |
| `git diff --check` | 没有冲突标记/空白错误 |
| `make -C test` | C 主机测试 + `test_web_auth.js` 全过 |

任何一项失败，脚本以非零状态退出；**不要提交这次同步**。先按下面的恢复步骤
把本地改动找回来，再重新解决冲突。

## 恢复

```bash
# 看同步前的备份
git tag -l 'pre-sync-*'
git show --stat pre-sync-20261116-070000

# 对比/恢复某个文件
git diff pre-sync-20261116-070000 -- html/app.js
git checkout pre-sync-20261116-070000 -- html/app.js

# 或者从补丁恢复
git am .local-records/pre-sync-20261116-070000/*.patch
```

如果用了 `--bundle`，也可以在任意仓库里：

```bash
git bundle verify .local-records/pre-sync-.../repo.bundle
git bundle unbundle .local-records/pre-sync-.../repo.bundle
```

本地标签/补丁只防“误覆盖/误 reset”，不防整机磁盘丢失；重要分支仍应
`git push fork <branch>`。

## 推荐顺序

```bash
git fetch origin
make pre-sync-check
git switch main
git merge origin/main          # 解决冲突时保留本地功能
make post-sync-check
git push fork main             # 异地备份
```

不要用 `git checkout origin/main -- .` 或 `git reset --hard` 跳过检查；如果真的
要整文件取上游，先确认 `post-sync-check` 会失败，然后把对应本地功能按 patch
重放。
