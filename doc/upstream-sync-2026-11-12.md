# 上游同步分析：RTLPlayground（2026-11-12）

| 项目 | 内容 |
|---|---|
| 本地 | `main` @ `5911eda`（2026-10-29），另有 19 个已改文件 + 10 个未跟踪项未提交 |
| 上游 | `logicog/RTLPlayground` `main` @ `eff1c55`（2026-11-11 16:08Z，PR #871） |
| 分叉点 | `03311de`（2026-10-13，PR #586） |
| 提交差 | 落后约 **579** 个提交（其中 10/13→10/28 共 269 个，10/28→11/11 共 310 个）；领先 47 个自有提交，`git cherry` 确认**全部未被上游收录** |
| 版本 | 本地 `VERSION=0.1.7`；上游 `VERSION=0.1.0`（未发布） |

> 取数说明：`git fetch` 在本机反复超时，本地对象只到 10/28；11/1–11/11 的提交与文件差异来自 GitHub compare API（`231b43b...main`，310 commits / 300 files）。因此 11 月新增的设备文件清单可能不完整，其余结论均基于实际代码与 diff。

---

## 0. 结论先行

1. **上游这一个月做了两件会长期影响你的事**：把 `machine.c` 拆成 `machine/<NAME>.c/.h`（PCB 命名规范 + 自动校验工具），以及把定时器从 `uip/` 抽成公共 `timer` 框架。前者意味着你 fork 里 6 个自研目标必须**重写而不是合并**；后者是底层改动，越晚合越贵。
2. **你的安全整改在上游仍然领先**：上游到 11/11 依旧是「空口令 = `POST pwd=` 即登录」、会话 48 bit、无 logout、Cookie 无 `HttpOnly`、口令明文存启动配置。这是你目前唯一能反向贡献且上游大概率接受的部分。
3. **冲突集中在 3 处**：`httpd/`+`sessions`（13 个上游提交 vs 你的 auth 改造）、`machine.c`/`machine_init.c`（上游删除，你新增目标）、`html/app.js`（上游 JS 重构 + API reshape vs 你的中文 UI 定制）。实测 `git merge-tree` 已预判 18 个文件冲突（见 3.1）。
4. **建议节奏**：先合 HTTP 健壮性修复并把安全改动适配到 `sessions.c`（P0）→ 再迁 `machine/`（P1）→ 最后跟 API reshape 与 RSTP/LACP（P2）。不要先动 `app.js`，上游 11/10 的 API reshape 会把 `status.json`/`mtu.json` 拆掉，现在改等于白改。

---

## 1. 上游新变化（按主题）

### 1.1 设备与命名重构（工作量最大）

| 上游动作 | 关键提交 / PR | 说明 |
|---|---|---|
| 每设备一对文件 | `a6b0918`、`7673ef8`、`c043c67` | `machine/<NAME>.c` + `machine/<NAME>.h`（可加 `_init.c`），`machine.c` / `machine_init.c` 被删除；`MACHINE_CHIPSET` 变为 `.h` 里的**必填**项，`MACHINE_MIN_PORT/MAX_PORT` 由芯片推导 |
| PCB 命名规范 | `f7760316`、`cc72dae6`、`fa200bd8`、`8fac3788`（#866） | PCB 名进 `MACHINE_*`，`-UNMANAGED` 后缀、`-`→`_`、`doc/pcb/` 重组、waiver 文件；`MACHINE_PCB_INFO` 进 `machine.h`，页面/CLI 可显示 PCB |
| 建板与校验工具 | `5106da6d`（#785）、`b9809b36`（#853） | `tools/add_device.py` 生成骨架、`check_device_docs.py` 校验 machine 与文档一致 |
| 新设备（10/13 之后，节选） | #742/#727/#671/#616/#723/#726/#765/#771/#784/#793/#794/#796/#809/#813/#814/#817/#820/#840/#844/#847/#849/#851/#855/#861/#865/#866/#868/#870/#871 … | ZoeRax HG0402XG、Sirivision S25G1109FP、GoodTop GT-ST024M、YuanLey YS25-0402M、MokerLink 2G08110GSN、hongyavision 2G5F4_10G2、Hasivo S1100P-8GT-1SX-S、Sodola SL902、DOTX DX-1009N、Ampcom WAM902、**XikeStor SKS3200M-8GPY1XF**、keepLINK KP-9000-6XHML-X2/9XHPML-X、Horaco ZX-SWTGW218AS、HC-SWTG124AS 等 |
| 工厂 MAC / reset 引脚 | `19c5ed2e`、`46dcdf8d`(#870)、`3762ee19`(#824)、`f015a38b` | 多块板从 flash 读出厂 MAC；reset 引脚默认上拉；SFP detect 修复 |

> 对你的直接影响：你的 6 个自研目标（`PCB_K0402W_V1`、`PCB_K0801W_V1`、`SKS3200M_5GPY1XF`、`SKS3200M_4GPY2XF`、`STEAMEMO_IG204_V1` 重写版、`F7005_2_5`）必须按新结构重写；其中 **`SKS3200M-8GPY1XF` 上游 11/4 也加了（#809）**，与你 fork 里的同名目标撞车，需要定一个为准。

### 1.2 定时器框架（底层）

`#761`（`54df1f7b`…`1dd39ec4`）：timer 库移出 `uip/`、可从任意 bank 调用、16 位、tick 率只设一次（`clock-arch.h`）；主循环用 timer 驱动 SFP/STP/秒计数；`delay/sleep`、DNS/NTP、DHCP、reset 按键、HTTP 请求超时、电缆测试、LACP、LLDP、SFP 上电全部改 timer。

**收益**：彻底摆脱对 `SEC_COUNTER`（秒寄存器）的隐式依赖——这正是你 fork 里自己修过的那类问题（`83d3df8`、`e49597c`、`c0d2de6`、`24b597b`）。**合上游后这些补丁可以直接丢弃**，是缩小分叉的好机会。

### 1.3 新功能（协议/页面）

| 功能 | PR | 备注 |
|---|---|---|
| RSTP + 重写 Spanning Tree 页 | #444 | `rtl837x_stp.c` 大改，带 `test/stp` 仿真测试；含 LAG 成员展示、p2p 设置（#732）、backup 口 designated（#744） |
| LACP 802.3ad | #299 | `rtl837x_lacp.c/.h`，含成员口链路跟随（#666）、fast rate |
| LLDP 增强 | #591/#600 | 上电立即发送（fast start）、org TLV、通告 PHY 状态/VLAN/聚合/帧长 |
| 电缆测试 | #576 | `port` 命令 + 页面 |
| MAC 计数 | #565 | “Devices learned” 列，每口统计 |
| 固件更新 CLI | #774 | `cli for firmware updates` |
| factory-reset 控制台命令 | #726 | **与你 fork 的 `restore factory defaults`（`ff51dc5`）功能重复** |
| `/information.json` 增加 uptime | #832 | 仪表盘显示 days + hh:mm:ss |

### 1.4 HTTP / Web

- **多会话**：单会话 → 4 会话（`#560`、`70f59b6`、`a7db0ca`、`e1c79ec`），会话代码抽到 `sessions.c/.h` 并放 BANK3；改口令时 `session_init()` 作废旧会话；登录带有效 Cookie 时复用槽位。
- **空闲超时 200s → 3600s**（11 月），页面隐藏时停止轮询。
- **健壮性**（都与你的安全改动同文件）：
  - `#568` 上传 boundary 的 CR 边界（`714c9eb`）
  - `#627` header 跨 TCP 段、方法名被切分、config 上传边界（`d104b31`、`2588965`）
  - `#727` JSON 数组闭合（`e53ebdd`）
  - `#582` 上传失败/复位归属（`491a919`、`674dc23`）
  - `#771` `/reset` 先应答再重启（`50ed730b`、`52819b7b`）
  - `#696` Firefox 密码自动填充（`0da6318`）
- **API reshape**（`#768`，11/10）：`/status.json` 拆成 `/ports.json` + `/traffic.json`；`/vlanlist`→`/vlanlist.json`；MAC 表改用**物理端口**（CPU = 物理 0）；`JSONQUOTE` 等辅助宏。**这是 breaking change**，你的 `app.js` 直接用到 `/status.json`、`/mtu.json`。

### 1.5 构建与 CI

| 变更 | 提交 | 影响 |
|---|---|---|
| 树内 SDCC 4.6.0 + 两个 mcs51 补丁（bug 4070） | `905e588`、`9cc859a`、`0181276`、`#649` | 工具链固定，跨机器可复现；你本地用系统 sdcc 会与上游产物/行为的差异变大 |
| `SOURCE_DATE_EPOCH` 默认取 HEAD 提交时间 | 上游已有 | **与你当前未提交的改动相反**（你改回“默认墙钟时间”）——合并时需明确取舍，否则 Makefile/CHANGELOG 必冲突 |
| `OUTPUT` / `OUTPUT_DIR` / `OUTPUT_PREFIX` / `OUTPUT_NAME` | #808 | 出包路径参数化 |
| `CONFIG=<file>` 每目标配置 | `e95e450`、`a573195` | 与你 fork 的 `config-xikestor.txt` 思路一致，可收敛 |
| `make bank_size` + `Werror` + debug 构建 CI | `94ca35f`、`148e868d`、`b0dfd9be` | 你的镜像已接近 BANK1 上限（你把 `rtl837x_igmp.c` 迁到 BANK3），这个工具要跟 |
| JS 压缩改 `rjsmin` | #716 | `app.js` 被格式化重写，diff 巨大（+5370/−2637），合并时几乎只能整文件取舍 |

### 1.6 其它

- **ChaCha20**（#207，`crypto/chacha20.c` + 8051 汇编）：目前是独立实现，未接入任何协议；8051 上做 TLS 仍不现实（你自己报告里的 P1-6 结论不变）。
- **Breaking changes**（上游 CHANGELOG 明示）：`port 0` 视为 CPU_PORT、多数命令不再接受 CPU_PORT、VLAN 不再接受 `u` 后缀。你的 Web UI/脚本/配置里若还有这些写法，合并后必须回归。

---

## 2. 你的 fork 与上游的重叠 / 冲突（实测）

### 2.1 `git merge-tree` 预判的冲突文件（18 个）

```
.gitignore  CHANGELOG.md  Makefile  README.md
doc/devices/ONT-S207CW-62TS-SE.md  doc/devices/SWTGW218AS.md  doc/supported_devices.md
html/app.js  html/index.html
httpd/httpd.c
machine.c  machine.h  machine_init.c  rtlplayground.c
test/Makefile  test/README.md  test/env_tables.c  test/test_httpd_tx.c  test/test_page_json.c
```

其中 `machine.c` / `machine_init.c` 是**删除 vs 修改**冲突（上游删了），不能靠 `-X ours/theirs` 糊过去。
关键文件差异量（HEAD ↔ origin/main）：

| 文件 | 增/删行 |
|---|---|
| `httpd/httpd.c` | 94 / 91 |
| `httpd/page_impl.c` | 61 / 162 |
| `html/app.js` | 481 / 489 |
| `cmd_parser.c` | 54 / 77 |
| `Makefile` | 133 / 97 |
| `machine.h` | 62 / 88 |
| `rtlplayground.c` | 88 / 190 |

### 2.2 已经重复造轮子的地方（合并时二选一）

| 你的实现 | 上游对应 | 建议 |
|---|---|---|
| `ff51dc5` restore factory defaults（命令 + 按钮） | `#726` `factory-reset` 控制台命令 | 保留你的按钮，命令名对齐上游，避免两套语义 |
| `SKS3200M_8GPY1XF`（fork） | `#809` `SKS3200M-8GPY1XF` | 以上游命名/结构为准，把你的实测差异补进去 |
| `SOURCE_DATE_EPOCH` 默认墙钟（你的未提交改动） | 上游默认确定性构建 | 建议跟随上游（CI/发布可复现），把“墙钟”作为 `SOURCE_DATE_EPOCH=$(date +%s)` 的用法写进文档 |
| `machine_name` / `model_name` 两行显示（`21a39a0`） | 上游 `MACHINE_PCB_INFO`（11/5，`f8af2855`） | 字段名对齐，避免 `struct machine` 里两套命名 |
| NTP 默认腾讯服务器 + UTC+8（`d931317`） | 上游无默认 | 保留（本地化合理），但注意上游 NTP 已重写，别重复实现 |

### 2.3 你领先上游、值得反向贡献的部分

| 内容 | 上游现状 | 价值 |
|---|---|---|
| **fail-closed 登录**（空口令不再放行） | 仍可 `POST pwd=`（空值）拿到会话（`is_url_word_x` 空串返回 true） | 安全，且改动小，最可能被接受 |
| 口令存 `passwd_hash <salt> <digest>`（salted SHA-256，浏览器算） | 明文存启动配置、明文进 `cmd_log` | 安全，但存储迁移大，建议先开 issue 讨论再 PR |
| 128 bit 会话 + 8h 硬过期 + 改密失效 | 48 bit（12 hex）、空闲 1h、改密清会话已有 | 可直接叠加到 `sessions.c` |
| `POST /logout` + 会话 Cookie `HttpOnly` | 无 logout、无 HttpOnly | 小改动，易接受 |
| 安全响应头（`X-Frame-Options: DENY`、CSP `frame-ancestors`、`nosniff`、动态端点 `no-store`、删 `Access-Control-Allow-Origin: *`、`X-RTLP` 防 CSRF） | 只有部分 CSP，静态资源仍有 `ACAO: *` | 小改动，建议按端点分批提 |
| **IG204 实测重写**（上游原为“never been measured”的占位） | 占位数据 | 硬件修正，上游最欢迎；但需按新 `machine/` 结构提交 |
| `F7005_2_5`（NPA AP-7024-2.5G）、`K0402W`/`K0801W`/`5GPY1XF`/`4GPY2XF` | 上游无 | 设备贡献，走 `add_device.py` + 新命名 |

---

## 3. 建议跟进顺序

### P0（本周，1.5–2 天）——先止血，且绕不开

1. **合上游 HTTP 层**：`httpd/`、`sessions.c/h`、`cmd_parser.c` 的修复与多会话。
   - 只取这些路径，不整树 merge，避免 `machine.c` 删除冲突提前爆发。
   - 合并后把 `auth.c` 从"自己实现会话"改成"在 `sessions.c` 上扩展"：4 槽位 + 128 bit + 8h 硬过期 + 改密 `session_init()`。
2. **把安全整改 rebase 到新结构上**（`doc/patches/0001-fail-closed-auth.patch` 需要重做）。
3. **顺手拿到上游这些稳定性修复**：上传 boundary/跨段 header/JSON 闭合/`/reset` 先应答/Firefox autofill。它们全是安全整改同文件的改动，晚合成本翻倍。

> 验收：`test/test_httpd_tx.c`（上游已有会话/上传回归用例）全绿 + 上板登录/改密/上传/重启 smoke。

### P1（2–4 周）——迁移，决定你能否继续跟上游

4. **`machine/` 迁移**：把 6 个自有目标写成 `machine/<NAME>.c/.h`（+ 必要 `_init.c`），`struct machine` 的 fork 扩展字段（`model_name`、`vlan_switch_pin`、`sys_led_blink_idle`）合并进 `machine.h`/`machine_defs.h`。
   - 用上游 `tools/add_device.py` 生成骨架、`check_device_docs.py` 校验文档；命名按 PCB 规范（`-UNMANAGED`、`_`）。
   - 设备文档迁到新目录约定，`doc/supported_devices.md` 按上游格式重排。
5. **构建对齐**：`SOURCE_DATE_EPOCH` 取舍、`OUTPUT_*`、`CONFIG=`、树内 SDCC、`make bank_size`。你镜像已接近 BANK1 上限，先跑 `bank_size` 看余量再决定是否保留 `auth.c` 的额外 RAM/CODE 占用。
6. **CLI breaking changes 回归**：CPU_PORT、VLAN `u` 后缀；把 `upgrade/*.sh`、`config*.txt`、Web UI 命令生成处全部过一遍。

### P2（之后）——跟功能，别自己造

7. **API reshape（#768）**：等 `app.js` 与后端一起改；短期可在 `httpd.c` 保留 `/status.json` 兼容端点一个月，避免你的中文 UI 大改。
8. **RSTP / LACP / LLDP / 电缆测试 / MAC 计数**：直接接受上游实现（有测试），只在中文翻译和页面上做增量。
9. **设备贡献 PR 化**：优先 IG204 实测修正 + F7005 + 你独有的 4 块板；每块一个 PR，按新命名，独立可审。

---

## 4. 落地步骤（命令级，建议）

```bash
# 0. 冻结现场（当前 19 改 + 10 未跟踪）
git switch -c wip/security-2026-11-12
git add -A && git commit -m "WIP: security hardening 2026-11-12 (baseline for upstream sync)"
git tag pre-upstream-merge-2026-11-12

# 1. 只合 HTTP/会话/命令层（手工挑路径，避免 machine 冲突）
git switch -c sync/http-2026-11
git checkout origin/main -- httpd/ sessions.c sessions.h cmd_parser.c cmd_parser.h html/app.js html/index.html
#   然后逐个 review：把 fork 的 auth 改动重放到 sessions.c 之上；app.js 用上游版 + 你的中文文案增量

# 2. 验证
make MACHINE=PCB_K0402W_V1 -j && make MACHINE=PCB_K0402W_V1 bank_size
make -C test            # 上游 host 测试套件

# 3. machine 迁移（新分支，逐设备）
git switch -c sync/machine-2026-11
#    以 machine/ 下同类板为模板写 PCB_K0402W_V1.c/.h 等；删除 machine.c/machine_init.c
#    把 model_name / vlan_switch_pin / sys_led_blink_idle 加进 machine.h + machine_defs.h

# 4. 全量合并（前两步都稳定后）
git switch main && git merge origin/main
#    冲突处理顺序：Makefile/CHANGELOG → httpd → machine.h → doc → html
```

**每个阶段的上板验收矩阵**（至少）：
- 冷启动 + DHCP/静态 IP 切换
- Web 登录（空口令被拒）/ 改密 / logout / 第二浏览器登录
- Save to flash + 断电重启配置保持
- 固件上传（含断线、错误镜像）
- 两个 SFP 笼 + 前面板 VLAN 拨钮（你的板特有）
- reset 按键 / factory reset

---

## 5. 不建议跟的

- **ChaCha20（#207）**：未接入任何协议，8051 上不能当 TLS 用；你报告里"固件内不做 TLS"的结论继续成立。
- **德语翻译、i18n 重构**：中文才是你的主用户，跟随成本 > 收益。
- **`app.js` 全量格式化（#716）**：一旦接受，你所有中文 UI 定制都要重做；建议等 API reshape 落地后整体重写一轮，而不是反复跟。
- **上游 `VERSION=0.1.0` 的发布节奏**：你 `0.1.7` 的自定义版本号与 `version.h` 语义要对齐，别再各写一套。

---

## 6. 风险清单

| 风险 | 说明 | 缓解 |
|---|---|---|
| BANK1 容量 | 上游 RSTP/LACP/timer/API 增长 + 你的 `auth.c` | 每步跑 `make ... bank_size`；必要时把 `sessions/auth` 再拆 BANK3 |
| SDCC 工具链差异 | 上游树内 4.6.0 + 补丁，你本地系统版 | 用上游 `make` 工具链目录，固定 `SDCC`/`sdcpp` |
| 设备数据回归 | `machine/` 迁移时容易丢 `flash_sio`/LED pad/端口映射 | 每块板迁移后与旧镜像逐项对比 `info`/`vlan`/`led` 输出 |
| 上传/配置兼容 | 上游要求行长度可被命令缓冲接收，否则拒写 | 迁移后回放你的 `config.txt`/`config-xikestor.txt` |
| 分叉继续扩大 | 上游日更 10–30 个提交 | 每月固定一次同步窗口；自有改动尽量 PR 化 |

---

## 7. 附：反向贡献清单（按优先级）

1. `httpd`: fail-closed 登录（空口令不授权）— 附带串口 provisioning 说明
2. `sessions.c`: 128 bit 会话 ID + 8h 硬过期 + `POST /logout` + `HttpOnly`
3. `cmd_parser.c` + `auth`: `passwd_hash <salt> <digest>`（需先讨论存储迁移路径）
4. `machine/`: IG204 实测修正（SFP detect/LOS、LED pad、VLAN 拨钮、SYS LED 空闲闪烁）
5. `machine/`: F7005_2_5 / K0402W_V1 / K0801W_V1 / SKS3200M_5GPY1XF / SKS3200M_4GPY2XF
6. `httpd`: 安全响应头（`X-Frame-Options`、`nosniff`、`no-store`、去 `ACAO: *`、`X-RTLP`）
