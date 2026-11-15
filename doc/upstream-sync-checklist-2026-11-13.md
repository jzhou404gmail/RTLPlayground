# 上游同步清单（2026-11-13）

> 动手前先跑 `make pre-sync-check`，合并后跑 `make post-sync-check`。前者会备份
> 本地提交并拒绝脏工作区，后者会检查退出登录/摘要口令/首装等本地功能是否被
> 覆盖。详见 [upstream-sync-guard.md](upstream-sync-guard.md)。

按「值不值得跟」排的三档。上游基线 `eff1c55`（11/11），本地分叉点 `03311de`（10/13）。
详细差异见 [upstream-sync-2026-11-12.md](upstream-sync-2026-11-12.md)。

## 一、立即同步（P0，1–2 天，改动小、收益大）

| # | 上游改动 | 代表提交 / PR | 为什么值得 | 适配成本 |
|---|---|---|---|---|
| 1 | 会话抽成 `sessions.c/h`：4 个槽位、BANK3、改密清会话、有效 Cookie 复用槽位 | #560 `70f59b6`、`a7db0ca`、`e1c79ec` | 你现在是单会话自研；改密失效、多浏览器互踢它都解决了 | 把 `auth.c` 改成调用 `sessions.c`，≈半天 |
| 2 | HTTP 健壮性：上传 boundary 的 CR、header 跨 TCP 段、方法名被切分、config 上传边界、JSON 数组闭合 | `714c9eb`(#568)、`d104b31`/`2588965`(#627)、`491a919`(#582)、`e53ebdd`(#727) | 全是 `httpd.c`，和你安全改动同文件；晚合成本翻倍 | 直接取，冲突少 |
| 3 | `/reset` 先应答再重启 | `50ed730b`、`52819b7b`(#771) | 你现在是 `delay(1000)` 硬等，丢包就重试 | 直接取 |
| 4 | Firefox 不再把保存的密码填进新口令框 | `0da6318`(#696) | 你刚加登录改密页面，正好吃这个坑 | 直接取 |
| 5 | 定时器框架：timer 库移出 `uip/`、主循环/延时/DHCP/NTP/reset 按键/HTTP 超时全改 timer | #761 `54df1f7b`…`1dd39ec4` | 你为「秒寄存器不可靠」打的 4 个补丁（`83d3df8`、`e49597c`、`c0d2de6`、`24b597b`）可整体删掉 | 中；与 `ntp`/`httpd` 改动交叉，先合底层再重放你的补丁 |
| 6 | 构建参数：`OUTPUT`/`OUTPUT_DIR`/`OUTPUT_PREFIX`/`OUTPUT_NAME`、`CONFIG=<file>`、`make bank_size`、`-Werror`(#763) | #808、`e95e450`、`94ca35f` | 你有 `config-xikestor.txt` 的多配置需求，正好收敛；BANK1 已接近满，`bank_size` 必须有 | 低 |
| 7 | 会话空闲超时 1 h（原 200 s）、标签页隐藏时停止轮询 | 11/2 前后 | 你 8 h 硬过期 + 上游 1 h 空闲，两者可以并存 | 低 |

> 建议顺序：先 2/3/4（纯 httpd 修复）→ 1（会话重构）→ 5（timer）→ 6/7。每步跑 `make -C test` + 上板 smoke。

## 二、迁移时同步（P1，2–4 周，结构性，绕不开）

| # | 上游改动 | 代表提交 / PR | 说明 |
|---|---|---|---|
| 8 | `machine.c` → `machine/<NAME>.c/.h`（+`_init.c`）、`MACHINE_CHIPSET` 必填、PCB 命名规范、`add_device.py`/`check_device_docs.py` | `a6b0918`、`7673ef8`、`c043c67`、`f7760316`、`cc72dae6`、#750/#764/#766/#785/#853/#866 | 不迁就跟不了任何新设备；你的 6 个目标要重写；`struct machine` 扩展字段（`model_name`/`vlan_switch_pin`/`sys_led_blink_idle`）并进新结构 |
| 9 | 与你硬件直接相关的修正 | SWTG024AS-V2.1.0 工厂 MAC `19c5ed2e`(#838)、reset pin `517087ec`(#784)、SFP detect `ffc50c4c`(#757)、reset 引脚上拉 `3762ee19`(#824)、F7008-2.5 SFP/LED `a9534e56`、XikeStor SKS3200M-8GPY1XF `255146a4`(#809) | 逐条对照你的板子；8GPY1XF 与你同名目标撞车，以上游命名 + 你的实测差异合并 |
| 10 | 树内 SDCC 4.6.0 + mcs51 补丁（bug 4070） | `905e588`、`9cc859a`、`0181276`(#649) | 固定工具链，避免你本地 sdcc 4.5.0 与上游产物行为不一致 |
| 11 | CLI breaking changes：`port 0` = CPU_PORT、多数命令不再接受 CPU_PORT、VLAN 取消 `u` 后缀 | #326/#334、上游 CHANGELOG | 合并后必须回归 `upgrade/*.sh`、`config*.txt`、Web UI 生成的命令 |
| 12 | 配置行长度校验：超长行拒写而不是写坏 flash | 上游 CHANGELOG「Config」段 | 和你「写回校验计数器」补丁是同一类，合并后只留一套 |

## 三、选择性同步（P2，跟上游实现，不要自研）

| # | 上游改动 | 结论 |
|---|---|---|
| 13 | RSTP（#444）、LACP（#299）、LLDP 增强（#591/#600）、电缆测试（#576）、MAC 计数（#565） | **接受**。都有 host 测试（`test/stp`、`test_lacp`），自己写不划算；只在中文翻译和页面布局上做增量 |
| 14 | API reshape（#768）：`/status.json` → `/ports.json` + `/traffic.json`，MAC 表改物理端口、CPU=物理 0 | **跟，但先别动**。等 `app.js` 一起改；过渡期可在 `httpd.c` 保留 `/status.json` 兼容端点，避免中文 UI 大改 |
| 15 | `factory-reset` 控制台命令（#726） | **对齐语义**：保留你的按钮，命令名/行为与上游一致，别维护两套 |
| 16 | `/information.json` 增加 uptime（#832） | **接受**，仪表盘顺带显示 |
| 17 | 德语 i18n、`app.js` 全量格式化/rjsmin（#716）、大量设备文档重组 | **跳过**。你的主用户是中文；`app.js` 定制多，整文件跟会白做 |
| 18 | ChaCha20（#207） | **暂不跟**。只是独立实现，未接入协议；8051 上 TLS 仍不现实 |

## 四、反向贡献（fork → 上游，缩小分叉）

| # | 内容 | 上游现状 |
|---|---|---|
| 19 | fail-closed 登录（空口令不授权 + 串口 provisioning 说明） | 仍可 `POST pwd=` 空值拿会话 |
| 20 | 128 bit 会话、`POST /logout`、Cookie `HttpOnly`、8 h 硬过期 | 48 bit、无 logout、无 HttpOnly |
| 21 | `passwd_hash <salt> <digest>`（浏览器侧 SHA-256） | 口令明文存启动配置、明文进 `cmd_log` |
| 22 | 安全响应头：`X-Frame-Options: DENY`、CSP `frame-ancestors`、`nosniff`、动态端点 `no-store`、去 `ACAO: *`、`X-RTLP` | 只有部分 CSP，静态资源仍 `ACAO: *` |
| 23 | IG204 实测重写（上游原为未测量的占位）、F7005/NPA AP-7024-2.5G | 占位数据 / 无 |
| 24 | K0402W-V1.0 / K0801W-V1.0 / SKS3200M-5GPY1XF / SKS3200M-4GPY2XF | 无 |

## 五、一句话

**跟「HTTP 会话与健壮性 + timer + 构建参数 + machine 结构」，接受 RSTP/LACP/LLDP/电缆测试，暂缓 API reshape 与 UI 重构，把安全整改反向推给上游。**
