# 上游 PR 计划（2026-11-14）

上游基线：`504ccc3`（main，11-13）。我们的分支：`sync/b1-api-reshape` @ `a78699a`。
目标：把 fork 里**通用**的改动拆成独立、可审的 PR，减少以后每次合并的冲突面。

## 一、已准备好、可直接开 PR

| # | 分支 | 内容 | 规模 | 验证 |
|---|---|---|---|---|
| 1 | `pr/sfp-module-flag` | SFP 空笼子语义：`enabled` 恒 true，新增 `module` 字段；UI 由链路状态决定 badge，端口详情显示「No module」 | page_impl + app.js + CHANGELOG，约 40 行 | 上游 main 上 `make MACHINE=STEAMEMO_IG204_V1` 构建通过；`make -C test` 全过 |
| 2 | `pr/login-password-toggle` | 登录页密码框内嵌「显示/隐藏」眼睛按钮（aria、6 语言标签、class 化样式） | login.html + CHANGELOG，约 60 行 | 构建通过 |

开 PR 链接（点开即 compare 页面）：

- https://github.com/logicog/RTLPlayground/compare/main...jzhou404gmail:RTLPlayground:pr/sfp-module-flag?expand=1
- https://github.com/logicog/RTLPlayground/compare/main...jzhou404gmail:RTLPlayground:pr/login-password-toggle?expand=1

## 二、建议后续提交的 PR（按依赖与风险排序）

| # | 主题 | 内容 | 规模 | 风险 / 备注 |
|---|---|---|---|---|
| 3 | 会话加固 | Cookie 加 `HttpOnly`、`POST /logout`、8 h 绝对上限、logout 只清当前浏览器 | sessions.c/h + httpd.c + UI 按钮，约 80 行 | 低；不依赖我们的摘要口令，可以独立落 |
| 4 | 安全响应头 | `X-Frame-Options: DENY`、`nosniff`、动态端点 `no-store`、去掉静态资源的 `ACAO: *` | httpd.c，约 20 行 | 低；去 ACAO 可能有人依赖，PR 里说明 |
| 5 | CSRF 头 | 写操作要求 `X-RTLP: 1`，UI 统一发送 | httpd.c + app.js + 文档，约 50 行 | 中；会破坏第三方脚本，需在 changelog 写“Breaking” |
| 6 | 登录节流 | 连续失败 1 s 起、翻倍到 32 s | httpd.c，约 30 行 | 低 |
| 7 | fail-closed | 空口令不再认证；串口 provisioning 流程 | httpd.c + 文档，约 20 行 | 中；属于策略变更，先开 issue 说明威胁模型 |
| 8 | 加盐摘要口令 | `passwd_hash <salt> <digest>`、`/auth.json`、浏览器端 SHA-256、旧明文回退 | auth.c/h 新增 + cmd_parser + httpd + login.html + app.js，约 600 行 | 高；建议先 RFC/issue 讨论存储迁移和复杂度 |
| 9 | 前面板 VLAN 拨钮 | `vlan_switch_pin` / `sys_led_blink_idle` 字段、pins 轮询、`vlan switch on/off`、JSON、卡片与只读锁定、IG204 板级定义 | machine.h + pins.c + rtlplayground.c + cmd_parser + page_impl + app.js + index.html，约 250 行 | 中；先开 issue 说明语义（照片 + `vlan switch` 行为） |
| 10 | 仪表盘布局与 SFP 配色 | VLAN 卡在流量上方、两列等高、SFP 端口块/表格行淡色区分 | index.html + app.js，约 40 行 | 低；依赖 #9 的卡片（可只提 SFP 配色） |
| 11 | 设备定义（逐个 PR） | `STEAMEMO_IG204_V1` 实测版（上游目前是**未实测占位**）、`F7005_2_5`、`PCB_K0402W_V1`、`PCB_K0801W_V1`、`SKS3200M_5GPY1XF`、`SKS3200M_4GPY2XF` | 每块板一个 `machine/*.c/.h` + 文档 | 上游采用 `pcb/` 共享定义结构，需按新命名搬迁；IG204 优先级最高 |

## 三、不建议上游化的部分

- `model_name` 字段：与上游「`machine_name` = 零售型号、`pcb_ver` = PCB 版本」的约定冲突；如要提，应作为单独的字段方案讨论。
- 每型号 fallback IP（`MACHINE_FALLBACK_IP`，10.12/10.247）：属于 fork 的现场策略。
- fork 的 Makefile 定制（XikeStor 配置映射、构建日期策略等）：与上游构建体系不兼容。
- 中文 README / 设备文档：上游已有自己的文档结构。

## 四、提交注意事项

1. 每个分支都从**上游 main** 拉，不要包含 fork 的 machine 树、`model_name`、fallback IP 等定制。
2. 一个 PR 只做一件事；更新 `CHANGELOG.md`（上游习惯），行为变化写进 Breaking。
3. 附主机测试结果；能在真机上验证的写上型号（如 IG204 / SKS3200M-5GPY1XF）。
4. 策略类改动（#5、#7、#8、#9）先开 issue，再贴 PR 链接。
5. 合并顺序建议：1 → 2 → 3 → 4 → 6 → 5 → 7 →（讨论 8/9）→ 11。

## 五、现状

- 已推分支：`pr/sfp-module-flag`、`pr/login-password-toggle`（均在上游 main 上构建/测试通过）。
- 本机没有 `gh` CLI，无法自动创建 PR；在上面的 compare 链接里点 “Create pull request” 即可。
