# 本 fork 功能清单与同步保护

这份文档和 [`tools/local-feature-checks.tsv`](../tools/local-feature-checks.tsv)
是同一个机制：**每一项 fork 自己的功能都必须在清单里有一行检查**。`make
post-sync-check` 会读取清单逐项检查；上游同步把这行代码覆盖掉，检查就失败，
不会等到上板或用户点按钮时才发现。

## 怎么加一项检查

1. 功能先实现，并尽量加测试。
2. 在 [`tools/local-feature-checks.tsv`](../tools/local-feature-checks.tsv) 加一行，
   TAB 分隔：

   ```text
   contains	html/index.html	id="sy-defaults"	maintenance has the factory-reset button
   absent	html/app.js	bytesToHex(sha256Hex	password digest is not double-hexed
   exists	test/test_auth.c	-	auth host test exists
   ```

   - `contains`：文件里必须出现这个字面量
   - `absent`：文件里不能出现这个字面量
   - `exists`：文件必须存在（第三列写 `-`）
3. 跑 `make local-feature-check`（快）或 `make post-sync-check`（含主机测试）。
4. 和功能代码一起提交。

同步流程：

```bash
git fetch origin
make pre-sync-check        # 拒绝脏工作区；给本地提交打备份标签
git merge origin/main
make post-sync-check       # 清单检查 + node --check + make -C test
```

`post-sync-check` 失败时**不要提交这次合并**；按
[upstream-sync-guard.md](upstream-sync-guard.md) 从 `pre-sync-*` 标签/补丁恢复。

## 恢复出厂设置（2026-11-16 确认）

### Web 维护页

System → Maintenance 卡片里有：

- **Reboot switch**
- **Restore factory defaults**

按钮在 [`html/index.html`](../html/index.html) 的 `#sy-defaults`，六语言文案和点击
处理在 [`html/app.js`](../html/app.js)。点击后确认，再向 `/cmd` POST
`defaults`（带 `X-RTLP: 1`），固件把镜像内嵌配置写回启动配置并重启。`defaults`
**不是**配置命令，故意不在 `CONF_CMDS` 里。

控制台等价命令：`defaults` 或 `factory-reset`。

### 长按 RESET

| 机型 | reset_pin | 上电时长按 10–30 s | 运行时长按 >10 s |
|---|---|---|---|
| XIKESTOR_SKS3200M_8GPY1XF（.201） | GPIO54 | `early_boot_handle_button()` 写默认配置，继续启动 | `handle_button()` 写默认配置并复位 |
| SKS3200M_4GPY2XF（.202） | GPIO54 | 同上 | 同上 |
| SKS3200M_5GPY1XF（.203） | GPIO54 | 同上 | 同上 |
| STEAMEMO_IG204_V1（.204） | `GPIO_NA` | 没有按键 | 没有按键；用 Web/`defaults` |
| PCB_K0801W_V1（.205） | `GPIO_NA` | 没有按键 | 没有按键；用 Web/`defaults` |

上电长按 >30 s 按原样继续正常启动；运行时长按 3–10 s 只复位、不恢复配置。
SKS 三台的 GPIO54 定义、`boot.c` 的 `early_boot_handle_button()` 和
`rtlplayground.c` 的 `handle_button()` 都在清单里，任何一项被同步覆盖都会在
`post-sync-check` 失败。

> 204/205 没有复位键，不要拿长按行为去要求它们；Web 按钮和 `defaults` 是这两台
> 的恢复路径。

### SKS 复位键实测结论（2026-10，已确认无需重测）

| 机型 | 实测日期 | ~5 s | 恢复出厂 | <3 s | 记录 |
|---|---|---|---|---|---|
| SKS3200M-8GPY1XF（.201） | 2026-10-13 | 重启，启动配置保留 | ~13 s，marker 被替换为按 MAC 生成的 hostname | 未单独重测，走同一代码分支 | [SWTGW218AS.md](devices/SWTGW218AS.md) |
| SKS3200M-4GPY2XF（.202） | 2026-10-12 | 重启，marker `RESETTEST4GPY` 保留 | ~11 s，marker 消失，只剩内嵌配置 | 无动作 | [SWTG024AS.md](devices/SWTG024AS.md#reset-button) |
| SKS3200M-5GPY1XF（.203） | 2026-10-12 | 重启，启动配置保留 | ~13 s，恢复内嵌配置 | 无动作 | [SKS3200M-5GPY1XF.md](devices/SKS3200M-5GPY1XF.md) |

这些测量是用 [`tools/reset_button_probe.py`](../tools/reset_button_probe.py)
配合启动配置里的 marker hostname 做的；原始 probe 日志没有留在仓库，结论和
marker 现象已写进上面的机型文档。

**代码是否变过：**

- 三个 SKS 的 `.reset_pin` 一直是 **GPIO54**；2026-11-14 的 `machine/` 拆分只是
  把定义搬了位置，没有改引脚。
- 阈值一直是：上电 **10–30 s** 恢复、>30 s 正常启动；运行 **>10 s** 恢复并复位、
  **3–10 s** 仅复位、**<3 s** 无动作。当前 `boot.c` 的
  `BOOT_HOLD_MIN_TICKS`/`BOOT_HOLD_MAX_TICKS` 和 `rtlplayground.c` 的
  `held_ticks > 10UL * SYS_TICK_HZ` / `> 3UL * SYS_TICK_HZ` 与实测时一致。
- 之后的改动只有计时实现：`9360097`（2026-10-30，boot 改用 timer 结构）、
  `1724911`（2026-11-14，timer 框架重构）。`handle_button()` 的差异只有
  “短按后 LED 恢复”这一行，复位动作和阈值未变。
- 因此只要 SKS 硬件/接线没有改动，**不需要为了这次 UI 修改再做物理长按测试**；
  清单会继续检查引脚和四个阈值，防止以后同步上游时被改掉。

### 出厂默认参数

所有 target 的内嵌配置都是：

```text
ip dhcp
passwd_hash 5f3c6ac1c12f81f3f1095f02cc65c05e 6fcabffe67fd0b86c1a199268e3fda929e0ccd3deeacf33f2389652853363967
```

第二行是口令 `1234` 的加盐 SHA-256 摘要（对应 `SHA-256(salt || "1234")`），
不是明文。`config.txt` 与 `config-xikestor.txt` 目前内容相同；Makefile 按
`XIKESTOR_TARGETS` 选择 `config-xikestor.txt`。

恢复出厂后：

| 项目 | 值 |
|---|---|
| 地址 | DHCP；没有 DHCP 时 XikeStor 三台为 `192.168.10.12/24`，其它机型为 `192.168.10.247/24` |
| 网关 / 掩码 | `192.168.10.1` / `255.255.255.0` |
| Web 端口 | `80` |
| 管理员密码 | **`1234`**（内嵌摘要；第一次登录后请立即修改） |
| Hostname | 清空后按机型/MAC 重新生成 |
| DNS / NTP / Syslog | 不写入启动配置；NTP/DNS 等按固件默认关闭/未设置 |
| 会话超时 | `3600` 秒（`SESSION_TIMEOUT_DEFAULT`） |
| 管理 VLAN | `1` |
| 端口名 / VLAN / 镜像 / 限速 / STP 配置 | 全部回到固件的初始默认 |

**重要**：201–205 现在都配置了 `http port 8000`、NTP、hostname 和各自的
`passwd_hash`。恢复出厂会全部清掉，Web 回到 **80**，口令回到默认 `1234`；不要
在远程不可达时随手点。恢复前先备份 `/config`（`upgrade/logs/pre-*` 里已有本轮
备份），恢复后按备份写回。

只有启动配置里完全没有凭据时（例如擦除配置或串口执行裸 `passwd`），登录页才
会显示首装页；恢复出厂不会走首装页，直接用 `1234` 登录。

### 8GPY1XF 的默认地址修正

`machine/XIKESTOR_SKS3200M_8GPY1XF.h` 之前缺 `MACHINE_FALLBACK_IP 12`，没有
DHCP 时会错误地落到 `192.168.10.247`。现在三台 SKS 都有：

```c
#define MACHINE_FALLBACK_IP 12
```

清单里有对应三行检查。

## 清单当前覆盖的 fork 功能

- 退出登录按钮、六语言文案、`X-RTLP` / `/logout`
- 改密轮换 salt、64 位摘要、`passwd_hash` 配置命令识别
- 首装页、旧明文回退、`/auth.json` 的 `legacy`
- 串口 `passwd_hash` 落盘、`auth_persist()` 保护
- 维护页恢复出厂按钮、`defaults` 后端、长按 RESET
- SKS 三台 `.12` 回落、IG204/K0801W 无按键
- 默认配置 `ip dhcp` + `1234` 摘要、无明文口令、Web 80、会话 3600、网关/掩码
- 主机测试与 `test_web_auth.js`

新增 fork 功能时，先加清单行，再加测试；这样下一次上游同步不会再把“自己的
需求”静默丢掉。
