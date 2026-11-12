# RTLPlayground 安全整改：代码修改建议

| 项目 | 内容 |
|---|---|
| 对应报告 | `RTL-SEC-2026-1112-01`（`RTLPlayground交换机安全整改报告_2026-11-12.md`） |
| 代码基线 | `5911eda` + 当前未提交修改（`git status` 与报告一致） |
| 日期 | 2026-11-12 |
| 配套补丁 | `doc/patches/0001-fail-closed-auth.patch`（已应用并部署，见第 11 节） |
| 部署状态 | 2026-11-12 08:10，四台已更新到 `v0.1.3-5911eda-dirty`，口令 `1234` |

---

## 0. 结论先行

1. **报告里性价比最高的一处修改是 `httpd/httpd.c:378-382`。** 所有敏感端点（`/config`、`/cmd`、`/upload`、`/reset`、`/cmd_log_clear`、全部 JSON）本来就检查 `authenticated`，只是被“空口令 = 已认证”整体绕过。删掉这个旁路，SEC-01 的入口一次性关闭。
2. **但这一行不能单独上线。** 改完后“无口令”等于“Web 永久 401”，新板或清过口令的板只能从串口恢复。必须同时写清楚 provisioning 流程（串口 `passwd` → Web 登录 → Save to flash），否则现场第一反应会是刷回旧固件。
3. **真正的治本是三件事**，顺序不能反：
   - 把口令从“可回读的启动配置”搬进独立的认证记录（先做存储迁移，再谈脱敏）；
   - 把 HTTP 命令面从“完整串口 CLI”收窄为白名单（认证 ≠ 最小权限）；
   - 固件先验签、再提交。
4. **不建议在固件里做 TLS**（8051 + uIP，RAM 与握手时间都不够）。报告 P1-6 应改成“管理 VLAN + 反向代理”，否则一定烂尾。
5. 报告的 P1 清单里，`/cmd` 全权限、改密不失效旧会话、`POST /config` 无校验、上传先写后验这四项**没有覆盖**，它们和 SEC-01 是同一类问题，建议并入同一批修复。

---

## 1. 复核：报告与代码的对应关系

### 1.1 判断正确的部分

| 报告结论 | 代码核对 | 结论 |
|---|---|---|
| 空口令直通 | `httpd.c:378-382`，`if (passwd[0] == NUL) authenticated = 1;` | 正确，且是总开关 |
| 端点级认证存在但被绕过 | `httpd.c:801-854`（POST）、`1021`（JSON）、`1080`（静态） | 正确 |
| 错误口令也发 Cookie | `httpd.c:705-721`，`passwd[0] == NUL \|\| ...` | 正确 |
| `GET /reset` 可重启 | `httpd.c:1071-1074` | 正确 |
| Cookie 48 bit | `httpd.c:14`，`SESSION_ID_LENGTH 12` | 正确 |
| `passwd` 明文入历史/配置 | `cmd_parser.c:1464-1484`、`2132-2147`、`httpd/page_impl.c:1085-1125` | 正确 |
| 固件只校验 CRC16 | `httpd.c:526-540` | 正确 |

### 1.2 需要修正的六点

**(1) 报告的 P1-1 补丁不完整。** 它把 `run_login_body()` 改成“口令非空且匹配才发 Cookie”，这是对的；但只改这两处后，`passwd[0] == NUL` 的设备**永远无法从 Web 登录**。报告正文提到了串口 provisioning，但补丁本身没有体现，评审时容易被当成“会把设备变砖”。建议把“串口 `passwd` + Web Save”写成补丁的一部分（本建议的 P0 就是这样做的）。

**(2) P1-2 的“配置脱敏”顺序有陷阱。** 现在的持久化链路是：

```
serial/Web 命令 → cmd_history → Web "Save to flash" → mergeConf(/config, /cmd_log) → POST /config
```

如果先让 `/config` 过滤 `passwd` 行，而 `passwd` 仍只存在于 `/cmd_log`/配置里，那么下一次“Save to flash”会把口令行**写丢**；重启后 `passwd[0] == NUL`，设备回到无口令。**必须先把口令写进独立认证记录（直接持久化），再脱敏。**

**(3) SEC-08（CORS / DNS rebinding）的优先级可以降。** 代码里 `Access-Control-Allow-Origin: *` 只加在静态资源上（`httpd.c:1096`），JSON 响应没有 ACAO，跨站 JS 本来就读不到；而 DNS rebinding 依赖“无认证”才能读响应，P0 修完后 cookie 绑定设备 IP，rebinding 页面拿不到会话。P1 里做 `X-Frame-Options`/`frame-ancestors`/去 ACAO 是低成本顺手项，不必和认证抢工期。

**(4) P1-6“固件加 HTTPS”不现实。** 8051 + uIP 没有 TLS 栈，证书存储、握手 RAM、熵源都是问题。正确做法：管理 VLAN + 管理主机上的 nginx/stunnel 反代；设备只监听管理 VLAN 的 8000。报告 6.3 的结论是对的，但 P1-6 的“固件/运维”拆分应改成“运维（反代）+ 固件（只做管理 VLAN 默认值）”。

**(5) P1-7“先验后写”需要存储前提。** 512 KiB 镜像放不进 RAM（xram 只有 48 KiB），无法“收完再验再写”。必须利用 A/B 或至少一个不活跃 bank：写入 staging → 校验签名 → 写提交标记 → 重启切换。代码里 `FIRMWARE_UPLOAD_START = 0x80000`（`rtl837x_common.h:106`）到底是什么语义（运行区还是暂存区），要先确认，否则“先验后写”无法落地。

**(6) “48 bit 会话”不是当前最紧迫的问题。** 攻击者现在不需要猜会话，直接无口令进门。但 P0 修完后它会立刻变成新短板，所以 128 bit 要跟 P0 一起上（本建议已包含）。

### 1.3 报告没有覆盖、但必须一起改的代码问题

| # | 问题 | 位置 | 影响 |
|---|---|---|---|
| A | `/cmd` 暴露完整串口 CLI：`passwd`、`defaults`、`http port`、`ip`、`mac`、`history` | `httpd.c:849-858` → `execute_commands()` | 认证后仍是全权限；`passwd` 走 HTTP 明文并进 `cmd_log` |
| B | 改/清口令不失效旧会话 | `cmd_parser.c:1464-1484` | 旧 Cookie 到空闲超时前一直有效；清口令后管理面状态不确定 |
| C | `POST /config` 接受任意文本，下次启动逐行执行，无命令/语法校验 | `httpd.c:404-471`、`cmd_parser.c:2169+` | 一次错误保存即可锁死管理面（如把 `http port` 写错），且是持久化执行通道 |
| D | 上传过程中就写 Flash，CRC 在最后才判定 | `httpd.c:500-560` | 校验失败/断线也会改写镜像区，失败不可回滚 |
| E | `passwd` 静默截断 20 字节、无长度/复杂度校验；`passwd ` 带尾空格会把空格存进口令 | `cmd_parser.c:1464-1484` | 弱口令、登录“明明输对了却进不去” |
| F | `is_url_word_x()` 把 `+` 当空格、支持 `%XX`，且比较非常量时间 | `httpd.c:191-233` | 口令不能含 `+`；存在理论上的时序侧信道 |
| G | `/config`、`/cmd_log` 无 `Cache-Control: no-store` | `httpd/page_impl.c:56` | 明文配置可能被浏览器/代理缓存 |

---

## 2. 目标设计（最小可信集）

### 2.1 认证状态机

```
                 serial: passwd <16-char random>
UNPROVISIONED ─────────────────────────────────► PROVISIONED
   (Web 全部 401)                                  │
                                                   │ 连续失败
                                                   ▼
                                              BACKOFF (1s→2s→…→30s)
                                                   │ 成功
                                                   ▼
                                              PROVISIONED

PROVISIONED ── serial/HTTP 改密 ──► 旧会话立即失效，仍是 PROVISIONED
PROVISIONED ── serial: passwd（无参数）──► UNPROVISIONED（Web 锁死，只能串口恢复）
```

关键规则：**从 UNPROVISIONED 到 authenticated，不存在任何网络路径。** 这也是 P0 补丁的核心。

### 2.2 三个入口，三套权限

| 入口 | 能做什么 | 口令 | 命令面 |
|---|---|---|---|
| 串口 console（`rtlplayground.c:1361`） | 全部 CLI | 可设置/清除 | 完整 |
| HTTP（`POST /cmd`） | 配置、查询、排障 | 只能通过 `POST /passwd`（校验旧口令） | 白名单，见 2.4 |
| 启动配置重放（`execute_config`） | 恢复配置 | **忽略 `passwd` 行** | 完整（信任存储） |

### 2.3 口令存储：从配置里搬走

现在的模型是“配置 = 启动时逐行重放的 CLI 命令”，`passwd` 只是其中一行，所以它必然出现在：

- `GET /config` 的响应里；
- `cmd_history` / `GET /cmd_log` 里；
- Web “Save to flash”的合并结果里；
- 任何配置备份/浏览器缓存里。

这就是为什么“只做脱敏”会破坏保存链路（见 1.2(2)）。建议新增一个 4 KiB 认证扇区（`0x71000`，紧接 CONFIG 之后，`FLASH_SECTOR_SIZE = 0x1000` 对齐）：

```c
#define AUTH_START 0x71000
#define AUTH_MAGIC "RTLA"

struct auth_record {           /* 按字节偏移显式序列化，不直接 memcpy */
    uint8_t  magic[4];         /* "RTLA"；擦除态 0xFF 即未 provisioning */
    uint8_t  version;          /* 1 */
    uint8_t  flags;            /* bit0 PROVISIONED，bit1 MANAGEMENT_DISABLED */
    uint8_t  reserved[2];
    uint8_t  salt[16];         /* provisioning 时由 get_random_32() 填 */
    uint8_t  hash[32];         /* SHA-256(salt || password) */
    uint32_t min_fw_version;   /* 防回滚，见第 7 节 */
    uint16_t crc;              /* CRC16(magic..min_fw_version) */
};
```

要点：

- `passwd` 命令**直接写这个扇区**，不再依赖浏览器 Save；串口 provisioning 一步到位，解决“串口改了 RAM、Web 才能保存”的鸡生蛋问题。
- `execute_config()` 启动时先 `auth_load()`；配置重放遇到 `passwd` 行**跳过并打印警告**，不再执行。
- 首次升级迁移：auth 扇区无效、但配置里有 `passwd <明文>` 时，把该明文哈希后写入认证记录，并把配置扇区里的该行过滤掉（用现成的 `config_buf` + `flash_write_bytes` 重写一次）。迁移失败就停在 UNPROVISIONED（安全侧失败）。
- 失败计数与锁定时刻只放 RAM（不进这个结构体）：写 Flash 会带来磨损，而且攻击者可以靠反复失败烧写寿命。锁定窗口 30 s 级别足够。

口令校验：

```c
/* SHA-256(salt || password)，32 字节摘要，恒定时间比较 */
uint8_t auth_check(const uint8_t *pw, uint8_t len)
{
    uint8_t d[32];
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, auth.salt, sizeof(auth.salt));
    sha256_update(&c, pw, len);
    sha256_final(&c, d);
    return ct_equal(d, auth.hash, 32) ? AUTH_OK : AUTH_DENIED;
}
```

- 8051 上一次 SHA-256（≤64 字节输入）是毫秒级，登录完全可接受；**不要**做 PBKDF2 迭代（1e4 次会让登录变成分钟级）。
- 口令策略：12–20 字符、可打印 ASCII、不含空格和 `+`（受 `is_url_word_x` 限制）、禁止空口令。
- 诚实说明：单次加盐 SHA-256 无法抵抗“拿到 Flash 后的离线爆破”。在 8051 上这是合理折中，前提是口令为 16 位随机串；物理读取 Flash 已在报告 1.4 的范围之外。

### 2.4 HTTP 命令白名单（认证 ≠ 最小权限）

`POST /cmd` 现在等于把串口 CLI 挂到网上。建议在 `execute_commands()` 里按来源分流：

```c
/* cmd_parser.h */
#define CMD_SRC_SERIAL 0
#define CMD_SRC_HTTP   1
#define CMD_SRC_CONFIG 2
extern __xdata uint8_t cmd_source;

/* httpd.c: run_cmd_body() */
cmd_source = CMD_SRC_HTTP;
execute_commands(body);
cmd_source = CMD_SRC_SERIAL;
```

```c
/* cmd_parser.c: execute_commands() 每行 cmd_parser() 之前 */
if (cmd_source == CMD_SRC_HTTP && !cmd_allowed_http(cmd_buffer)) {
    print_string("Not available over HTTP; use the serial console\n");
    err_status = ERR_NOT_ALLOWED;
    return;
}
```

白名单与 `html/app.js` 的 `CONF_CMDS`（`app.js:866` 一带）**同源**，用脚本生成 C 表，避免两边漂移。HTTP 侧明确禁止：

- `passwd`（改走 `POST /passwd`）
- `defaults`（清配置）
- `http port`（改管理端口，最典型的自锁操作）
- `history`（泄露历史命令）
- `mac`（改管理 MAC，UI 未使用）

其余 `ip/gw/netmask/vlan/pvid/port/stp/...` 保留，因为 Web UI 本来就在用；它们已是“管理员权限”的正常配置面。

### 2.5 会话

- `SESSION_ID_LENGTH` 32（128 bit），每次登录重新生成；
- Cookie：`Path=/; HttpOnly; SameSite=Strict`，有 TLS 时再加 `Secure`；
- 空闲超时保留（`session` 命令可配），增加 8 h 绝对超时；
- `POST /logout`：清 `session_id` + `Set-Cookie: session=; Max-Age=0`；
- 改密/清口令时调用 `httpd_forget_sessions()`；
- 失败退避：连续 5 次后 1 s、2 s、4 s…上限 30 s，成功即清零；事件写审计。

### 2.6 为什么不建议在固件里做 TLS

- uIP 没有 TLS，接入 mbedTLS 需要至少 30–50 KiB RAM 和可观的 code bank，8051 上握手时间以十秒计；
- 证书/私钥没有安全存储位置；
- 换来的收益可以用“管理 VLAN + 反代”低成本获得。

如果一定要 HTTPS：在管理主机放 nginx/stunnel，交换机只监听管理 VLAN 的 8000，并只放行反代地址。等 P1 的 `Host`/`Origin` 校验做完，反代方案的可信度足够。

---

## 3. P0（今天就能上）：fail-closed + 会话生命周期

> 2026-11-12 08:10：本补丁已应用到工作区并部署到四台设备，结果与证据见第 11 节。

补丁：`doc/patches/0001-fail-closed-auth.patch`，改动 3 个文件、约 45 行：

| 文件 | 改动 |
|---|---|
| `httpd/httpd.c` | 删除空口令旁路；`run_login_body()` 要求口令非空；Cookie 加 `Path=/; HttpOnly`；会话 12→32 hex；增加 8 h 绝对超时；新增 `httpd_forget_sessions()` |
| `httpd/httpd.h` | 声明 `httpd_forget_sessions()` |
| `cmd_parser.c` | `passwd` 设置/清除后失效当前会话 |

验证情况：`git apply --check` 通过；`httpd.c` 用 `test/sdcc_shim.h` 在 host gcc `-Werror` 下编译通过；`cmd_parser.c` 无法 host 编译（`8051.h`），改动仅为两处调用 + 一个 `__banked` 声明。

### 3.1 补丁做了什么

```c
/* httpd.c: scan_header() */
-	if (passwd[0] == NUL)
-		authenticated = 1;
+	if (passwd[0] == NUL)
+		authenticated = 0;
```

```c
/* httpd.c: run_login_body() */
-	if (passwd[0] == NUL || (strstart(body, "pwd=") && is_url_word_x(body + 4, passwd))) {
+	if (passwd[0] != NUL && strstart(body, "pwd=")
+	    && is_url_word_x(body + 4, passwd)) {
 		...
-		slen += strtox(outbuf + slen, "; SameSite=Strict\r\n\r\n");
+		slen += strtox(outbuf + slen, "; Path=/; HttpOnly; SameSite=Strict\r\n\r\n");
```

### 3.2 上线步骤（四台逐台做）

1. 接串口，确认能看到 CLI 提示符；记录当前 `/config`（用带 Cookie 的方式，或先按报告基线备份）。
2. 刷 P0 固件 → 重启。此时 Web 全部 401，**这是预期行为**。
3. 串口执行 `passwd <16 位随机串>`（每台唯一，入密码库）。该命令会写 RAM 并记入 `cmd_log`。
4. 浏览器登录 `http://<ip>:8000/`，用新口令登录；点“Save to flash”把 `passwd` 行写进启动配置（P1 之前仍需这一步）。
5. 验证：

```bash
# 未认证：期望 401
curl -s -o /dev/null -w '%{http_code}\n' -X POST http://IP:8000/cmd --data version
curl -s -o /dev/null -w '%{http_code}\n' http://IP:8000/config

# 错误口令：期望 302 -> login.html，且没有 Set-Cookie
curl -s -D- -o /dev/null -X POST http://IP:8000/login --data 'pwd=wrong'

# 正确口令：期望 302 + 32 hex + HttpOnly/SameSite
curl -s -D- -o /dev/null -X POST http://IP:8000/login --data 'pwd=REAL'
```

6. 重启一次，重复第 5 步，确认不是“只改了 RAM”。
7. 从 IoT/无线侧 `nc -zv IP 8000`，期望超时（配合 P0-1 隔离）。

### 3.3 副作用与回滚

- **Web 侧“清空口令”语义变了**：`passwd`（无参数）不再是“打开管理面”，而是“Web 锁死、只能串口恢复”。这是有意的 fail-closed，CLI 帮助文字（`html/app.js:114`、`360` 的 `sy_cmdref_text`）要同步改。
- 新板/恢复出厂后，必须先串口设口令，否则 Web 不可用；`login.html` 建议增加“未 provisioning”的提示（`/login` 可 302 到 `login.html?e=np`）。
- 回滚：刷回旧固件即可；但注意旧固件会重新打开管理面，回滚窗口必须处于报告 P0-1 的隔离网络中。

---

## 4. P1：口令与最小权限（治本的第一步）

### 4.1 新增模块

| 文件 | 内容 | 预估 |
|---|---|---|
| `sha256.c/.h` | 流式 SHA-256（init/update/final），`__code` 常量表 | ~250 行 |
| `auth.c/.h` | `auth_load/save/check/set_password/note_failure/backoff`，认证记录 CRC | ~200 行 |
| `httpd/httpd.c` | `POST /passwd`、`POST /logout`、会话绝对超时、Cookie 属性 | ~80 行 |
| `cmd_parser.c` | `passwd` 改走 auth 记录；`cmd_source` 白名单；历史跳过 `passwd` | ~60 行 |
| `html/app.js`、`html/index.html` | 改密表单改调 `/passwd`；`api()` 加 CSRF 头；`/reset` 改 POST | ~40 行 |

`passwd` 命令的新语义：

```c
void parse_passwd(void)
{
    /* 只有串口可以“无旧口令”设置；HTTP 走 POST /passwd，必须验旧口令 */
    if (cmd_words_len == 1) {          /* 清空 = 禁用 Web 管理，串口恢复 */
        auth_clear();
        httpd_forget_sessions();
        print_string("Web management disabled; provision over the serial console\n");
        return;
    }
    ...
    if (!auth_set_password(pw, len)) { cmd_error("Password must be 12-20 chars\n"); return; }
    httpd_forget_sessions();
}
```

`POST /passwd`：`old=<...>&new=<...>`，要求已认证、校验旧口令、12–20 字符、写认证扇区、失效所有会话、返回 204；失败计入退避。

### 4.2 脱敏（必须在 4.1 之后）

- `send_config()`：跳过以 `passwd` 开头的行（迁移完成后配置里本来就不该有）；
- `send_cmd_log()`：跳过 `passwd` 行（或在写历史时就不记录）；
- `app.js` 的 `CONF_CMDS` 删除 `/^passwd\s+\S+$/`，`mergeConf` 不再可能把口令带进配置；
- 加 `Cache-Control: no-store` 到 `/config`、`/cmd_log`、`/status.json`、`/information.json`。

> 顺序警告：4.1 没做完就做 4.2，会导致“Save to flash”把口令行写丢，重启后回到无口令（见 1.2(2)）。

### 4.3 登录退避

```c
/* auth.c */
uint8_t auth_backoff_active(uint32_t now) { return now < auth_lock_until; }
void auth_note_failure(uint32_t now)
{
    if (auth.fails < 255) auth.fails++;
    if (auth.fails >= 5) {
        uint8_t sh = auth.fails - 5; if (sh > 5) sh = 5;
        auth_lock_until = now + (1UL << sh);   /* 1,2,4,8,16,32 s */
    }
}
```

成功后 `auth.fails = 0`。不要做“永久锁定”，否则构成对管理员的 DoS；30 s 上限 + syslog 告警足够。

---

## 5. P1：Web 面加固

### 5.1 状态变更端点改 POST

`/reset`、`/cmd_log_clear`、`/l2_del.json` 现在都是 GET（`httpd.c:1047-1074`）。改成 POST，并在 `handle_post()` 里复用同一套认证检查；GET 返回 405。`index.html`/`app.js` 的调用点同步改。

### 5.2 CSRF / DNS rebinding

在 `api()` 里给所有非 GET 请求加 `X-RTLP: 1`，服务端对 `POST /cmd`、`/config`、`/upload`、`/passwd`、`/reset`、`/logout` 强制要求该头：

- 跨站表单/fetch 无法带自定义头（会触发 preflight），CSRF 直接失效；
- DNS rebinding 页面即使同源，也拿不到设备域下的 Cookie（Cookie 绑定设备 IP），加上 `Host` 校验是双保险；
- `Host` 校验：只接受“当前管理 IP 的字面量”或配置的 `hostname`（`boot.c:466` 的 `hostname` 全局），其余 421。

### 5.3 响应头

在 `httpd.c:1085-1096` 的静态响应头基础上补：

```
X-Frame-Options: DENY
Content-Security-Policy: ...; frame-ancestors 'none'      (已有 CSP，追加即可)
X-Content-Type-Options: nosniff
Referrer-Policy: no-referrer
Cache-Control: no-store                                    (仅敏感端点)
```

移除 `Access-Control-Allow-Origin: *`。注意 CSP 里已有 `unsafe-inline`（登录页内联脚本需要），保持现状即可，不要为了“更安全”把它删掉导致登录页失效。

### 5.4 审计钩子

`syslog.c` 现有的是“把 CLI 输出顺带进 ring”的模型，不适合登录失败这类事件（会污染 HTTP 响应）。加一个只写 ring 的入口：

```c
void audit(const char *msg);   /* 直接追加到 logbuf，不经过 write_char */
```

事件：登录成功/失败（源 IP）、口令变更、配置写入、固件上传/验签结果、重启、清日志。`/cmd_log_clear` 改为“需认证 + 先记录谁在何时清空”。

---

## 6. P1/P2：固件签名与防回滚

镜像尾部追加 512 字节签名块（保持 512 KiB 镜像尺寸不变）：

| 偏移 | 字段 | 说明 |
|---|---|---|
| 0x00 | magic `"RTLP"` | 识别签名块 |
| 0x04 | version u32 | 单调递增，用于防回滚 |
| 0x08 | machine name[30] | 与 `machine_name`/`hw_ver` 绑定 |
| 0x26 | image_len u32 | 被签名的字节数 |
| 0x2A | sha256[32] | `image[0..image_len)` 的摘要 |
| 0x4A | sig[256] | RSA-2048 PKCS#1 v1.5 over sha256 |
| 0x14A | crc16 | 签名块自身完整性 |

选 RSA 而不是 Ed25519 的原因：8051 上 Ed25519 **验签**需要完整标量乘，代码量和时间都不划算；RSA-2048 用 `e=65537` 验签只有 16 次模平方 + 1 次模乘，bignum 实现约 3–5 KiB code，耗时可接受。

校验流程（在现有 `stream_upload()` 收尾处）：

1. 流式写 staging 的同时算 SHA-256 和 CRC16（CRC 保留兼容）；
2. 收完最后一个 boundary 后，校验 magic/型号/版本下限/签名；
3. 全部通过：写提交标记（认证扇区或镜像头），再 `reset_chip()`；
4. 任一失败：擦除 staging 头、不重启、返回 400；**不能再出现“CRC 失败但字节已落盘”**。

防回滚：`min_fw_version` 存认证扇区（不会被 `defaults` 清掉），只在验签通过的升级后更新；启动时比较镜像版本。

前置确认：`0x80000` 是运行区还是暂存区、Bootloader 是否已经有验签。如果 Bootloader 不做校验，那么“HTTP 验签”只是第一道门，能写 Flash 的人仍可绕过；这部分必须和硬件/引导方确认，不能只改应用层。

---

## 7. P2：配置校验、L2 默认值、构建

- **配置上传校验**：`config_take()` 现在只检查行长和总长。加“逐行 tokenize + 白名单 + 拒绝 `passwd`/`defaults`/`http port`”，失败整包拒绝（不写 Flash），避免一次误保存把设备锁死。
- **L2 默认值**：`stp on` + 接入端口 `guard bpdu`、上行 `guard root`、风暴抑制阈值、闲置端口 `off`、VLAN 1 不作管理。这些都能用现有 CLI 组合，建议做成 `defaults` 的配置模板（`config.txt`）而不是硬编码。
- **构建**：去掉 `-dirty`；`make` 记录 commit、工具链、SBOM；发布镜像带 SHA-256 与签名。

---

## 8. 测试

### 8.1 host 单测（`test/` 已有框架，成本最低）

| 新增 | 用例 |
|---|---|
| `test_httpd_auth.c` | 无 Cookie `GET /config`→401；无 Cookie `POST /cmd`→401；`POST /upload`→401；错误口令→302 且无 `Set-Cookie`；正确口令→32 hex + `HttpOnly` + `SameSite`；会话超时/绝对超时→401；`passwd` 后旧 Cookie→401 |
| `test_auth.c` | SHA-256 已知答案；`ct_equal` 相等/不等；擦除态记录→UNPROVISIONED；CRC 损坏→UNPROVISIONED；改密后旧 hash 失效；退避窗口边界 |
| `test_config_policy.c` | `POST /config` 含 `defaults`/`http port`/超长行→拒绝且不写 Flash；正常配置→写回一致 |

`test_httpd_tx.c` 已经能把 `httpd.c` 原样编进 host，新增认证用例可以直接复用它的环境（`test_httpd_tx.c:64-160` 的 mock 全局）。

### 8.2 设备验收

直接复用报告第 4 节的 A-01～A-18，补三条：

- A-19：`passwd` 后旧 Cookie 立即 401（会话失效）；
- A-20：连续 5 次错误口令后出现退避；重启后计数清零，但口令仍在，Web 仍 401；
- A-21：`POST /config` 写入非法命令被整包拒绝，设备仍可登录。

### 8.3 灰度顺序

`.204`（端口最少、影响最小）→ `.203` → `.202` → `.201`；每台间隔至少一次重启验证。全程保留串口旁路。

---

## 9. 工作量与提交拆分

| 提交 | 内容 | 预估 | 依赖 |
|---|---|---|---|
| P0 | `0001-fail-closed-auth.patch` + 上线验证 | 0.5 天 | 串口可用 |
| P1-a | `sha256.c` + `auth.c` + `passwd` 改存储 | 2 天 | P0 |
| P1-b | `POST /passwd` + UI + 脱敏 + `no-store` | 1.5 天 | P1-a |
| P1-c | HTTP 命令白名单 + 配置上传校验 | 1.5 天 | P1-a |
| P1-d | 会话/CSRF/响应头/POST 化 | 2 天 | P1-b |
| P1-e | 审计事件 + `/cmd_log_clear` 保护 | 1 天 | P1-d |
| P1-f | 固件签名 + 防回滚 | 4–6 天 | 确认 Bootloader/暂存区语义 |
| P2 | L2 默认值、构建、SBOM | 2 天 | P1 |

总计约 14–16 人天；如果只能投 3 天，做 P0 + P1-a + P1-b，能把“无口令 + 明文口令外泄”这条主链断掉。

---

## 10. 明确不建议的做法

1. **在 8051 固件里实现 TLS。** 成本高、收益低，用管理 VLAN + 反代替代。
2. **在 `app.js` 里做权限控制。** 现在的 `CONF_CMDS` 只是“脏标记”判断，不是安全边界；白名单必须在服务端。
3. **只脱敏、不迁移口令存储。** 会让 Save 丢掉口令（见 1.2(2)）。
4. **用 HMAC + 内置密钥代替签名。** 密钥在公开固件里，等于没有。
5. **保留“空口令 = 放行”的兼容开关。** 这个开关一旦存在，就一定会被打开。
6. **一次性大重构认证/配置模型。** 先 P0 止血，再按上面的提交粒度走，每步都能单独回滚。


---

## 11. 实施记录（2026-11-12）

| 设备 | 型号 | 构建 | JEDEC | 结果 |
|---|---|---|---|---|
| 192.168.32.201 | SKS3200M-8GPY1XF | `SKS3200M_8GPY1XF` | `ef4015` | OK |
| 192.168.32.202 | SKS3200M-4GPY2XF | `SKS3200M_4GPY2XF` | `ef4015` | OK |
| 192.168.32.203 | SKS3200M-5GPY1XF | `SKS3200M_5GPY1XF` | `ef4015` | OK |
| 192.168.32.204 | IG204 V1 | `STEAMEMO_IG204_V1 FLASH_NO_DIO=1` | `1c4116` | OK |

- 镜像：`v0.1.3-5911eda-dirty`（`Makefile` 的 `VERSION` 从 0.1.1 提到 0.1.3）。后缀仍是 `-dirty`，因为工作区还有本补丁之外未提交的修改。
- 部署顺序：先在旧固件上把 `passwd 1234` 写进启动配置并回读，再刷写；否则新固件 fail-closed 后无口令会锁死 Web，而现场没有串口。
- `.204` 必须用 `FLASH_NO_DIO=1`（EON `EN25QE32A`，4 MiB，JEDEC `1c4116`）；其余三台是 Winbond 2 MiB（`ef4015`），DIO 构建。
- 出厂默认配置：`upgrade/config/default-p0.txt`（`ip dhcp` + 实验室 DNS/NTP + `http port 8000` + `passwd 1234`），通过 `DEFAULT_CONFIG=` 编译进 `0x6f000`/`0x70000`，四份镜像都做了字节级确认。恢复出厂后 Web 仍在 8000、口令 1234，但 hostname 会回到 `RTLPlayground-xxxxxx`。
- 验收（`upgrade/logs/p0-verify-20261112-081008.txt`）：四台未认证 `info/config/cmd/upload/reset/cmd_log_clear` 全部 401；错误口令与空口令均不返回 `Set-Cookie`；`1234` 登录返回 32 hex + `HttpOnly` + `SameSite`；配置与 VLAN 和部署前逐行一致（只多 `passwd 1234`）；链路状态与报告基线一致；NTP 已同步；网关可达。
- 复现命令（注意：不带 `DEFAULT_CONFIG=` 重新构建会丢掉出厂默认口令）：

```bash
make MACHINE=SKS3200M_8GPY1XF DEFAULT_CONFIG=upgrade/config/default-p0.txt -B -j4
make MACHINE=SKS3200M_4GPY2XF DEFAULT_CONFIG=upgrade/config/default-p0.txt -B -j4
make MACHINE=SKS3200M_5GPY1XF DEFAULT_CONFIG=upgrade/config/default-p0.txt -B -j4
make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1 DEFAULT_CONFIG=upgrade/config/default-p0.txt -B -j4
```

- 证据：`upgrade/logs/p0-config-before-*`、`p0-config-seeded-*`、`p0-seed-*.log`、`v011-flash-*.log`、`p0-verify-*.txt`。部署脚本：`upgrade/p0-seed.sh`、`upgrade/p0-verify.sh`。

### 11.1 仍然存在的风险（P0 不解决）

1. **`1234` 是 4 位口令，且 P0 没有失败计数/退避**：同网段可以在线爆破，几分钟量级。P1-3 之前建议换成 16 位随机串（每台唯一）。
2. **口令仍是明文**：`/config`、`/cmd_log` 在认证后可读到 `passwd 1234`，HTTP 也是明文；P1-2 前不要把这四台的管理面暴露给不可信网段。
3. **`/cmd` 仍是完整 CLI**（`defaults`、`http port`、`passwd` 都能执行）；`POST /config` 仍接受任意命令文本；固件仍无签名。这些都按第 4～7 节处理。
4. **构建仍带 `-dirty`**；`v0.1.3` 是本次手写的版本号，不代表一个可复现的干净 tag。

---

## 12. 第二轮（v0.1.4，2026-11-12 10:40）

四台已更新到 `v0.1.4-5911eda-dirty`（构建时间 10:49–10:51），启动配置、VLAN、端口 link 与 v0.1.3 一致；`igmp show` 等 CLI 已回归（IGMP 移到 BANK3 后代码/常量段一起迁移）。

| 复评项 | 状态 | 说明 |
|---|---|---|
| 构建时间 | **完成** | `BUILD_DATE` 改为编译时墙钟；四台显示 `2026-11-12 10:49–10:51`。需要可复现构建时显式传 `SOURCE_DATE_EPOCH`。 |
| R-03 登录限速/最小长度 | **完成** | 连续 5 次失败后 1 s、2 s、4 s…最多 32 s；运行期 `passwd` 要求 6–20 位；启动配置里的旧口令（4 位）仍可登录。 |
| R-04 CORS/Host/XFO | **完成** | 移除 `Access-Control-Allow-Origin: *`；`Host` 只接受本机 IP/主机名（否则 421）；`POST /cmd`、`/config`、`/upload` 要求 `X-RTLP: 1`（否则 403，未认证仍是 401）；`X-Frame-Options: DENY`、CSP `frame-ancestors 'none'`、`nosniff`、`no-store`。 |
| R-01 纯 HTTP | **部分** | 固件侧能做的已做：`http tls on` 让 Cookie 带 `Secure`，供 TLS 反代使用；固件本身仍是 HTTP。真正的 TLS 需要管理 VLAN + 反代（8051 上不跑 TLS）。HSTS 为节省 BANK1 空间未内置，由反代添加。 |

### 12.1 R-02（口令明文）为什么没在这一版完成

尝试过在固件里做加盐 SHA-256，结论是**当前 8051 的内存/代码空间放不下一个可安全上线的实现**：

- 基线内部 RAM（DSEG）已用 90/128 字节，只剩约 30 字节；SHA-256 + 认证记录需要约 41 字节，直接链接失败。
- 把 SHA/认证/HTTP 认证代码改用 `--model-large`（变量进 xdata）并把 IGMP 移到 BANK3 后，RAM 能过、BANK1 仍超 1.5 KB；继续裁掉 HSTS/Referrer-Policy 等响应头后勉强塞进 BANK1。
- 但这样得到的是**混合内存模型**镜像，`sdld` 报 `Conflicting sdcc options`，跨模块指针 ABI 没有经过设备验证。现场没有串口，四台又是生产/实验网设备，不能拿一个带链接警告、未验证的镜像去刷。
- 因此 v0.1.4 保留明文口令（但 `passwd` 已不再写入 `cmd_log`）；`/config` 仍会返回 `passwd` 明文。

可选路径（需要确认后实施）：

1. **主机侧哈希（pass-the-hash）**：固件只存 32 字节摘要，登录时浏览器/脚本计算 `SHA-256(salt||password)` 后提交；固件零 crypto，RAM 足够。代价：摘要本身是可重放的凭据，且要改登录页/脚本。
2. **字节级 SHA-256 移植**：用 8 位运算实现，DSEG 可压到几个字节，但代码量/耗时增加，需要完整回归。
3. **接受明文 + 管理 VLAN/物理隔离**：把残余风险限制在管理网，P1 再处理。

在选定之前，建议至少把 `/config` 的读取权限限制在管理 VLAN 内，并尽快轮换 `1234`。

### 12.2 运维注意事项

- 所有会 POST `/cmd`、`/config`、`/upload` 的外部脚本都要加 `-H 'X-RTLP: 1'`；`upgrade/*.sh` 已更新。
- `Host` 校验只接受“当前管理 IP”或 `hostname`；通过其他域名/反代访问会被 421。
- 登录连续失败会临时退避，属预期；等待 30 s 内即可恢复。


### 12.3 本轮验收

- 未认证端点全部 401；错误/空口令无 Cookie；`1234` 登录正常（32 hex、HttpOnly、SameSite）。
- `Host: evil.example.com` → 421；`POST /cmd` 无 `X-RTLP` → 403、带 → 200；未认证仍是 401。
- `/login.html` 带 `X-Frame-Options: DENY`、CSP `frame-ancestors 'none'`、`nosniff`，无 `Access-Control-Allow-Origin`。
- 配置/VLAN/端口 link 与升级前一致；网关可达；`.201` 升级后 NTP 偶发未同步（DNS 解析失败，需等下一轮），其余三台正常。
- 日志：`upgrade/logs/v014-verify-20261112-105630.txt`、`upgrade/logs/v014-security-check-20261112-105653.txt`。


---

## 13. v0.1.7 / .204 试点（2026-11-12 11:22）

`192.168.32.204` 已单独升级到 `v0.1.7-5911eda-dirty`，其余三台仍是 `v0.1.4`。

### 13.1 口令方案（主机侧哈希）

- 启动配置只保留 `passwd_hash <32 hex salt> <64 hex digest>`，不再有明文 `passwd`。
- 登录页从公开的 `GET /auth.json` 取 salt，用内置 JS SHA-256 计算 `SHA-256(salt || password)`，只提交 64 位十六进制摘要。
- 固件只做常量时间比较，不做哈希；`passwd` 不再接受明文参数，改密走 Web UI（生成新 salt + 摘要，`passwd_hash` 命令写回配置）。
- 迁移过程保留了一行明文 `passwd 1234` 作为回退，确认摘要登录可用后已删除；现在 `.204` 的配置里没有明文。

**代价**：摘要是可重放的凭据（pass-the-hash）。在纯 HTTP 下与嗅探明文口令等价；`/config` 对已认证用户仍会显示摘要，`1234` 的摘要可被离线爆破。建议评估通过后把口令换成 16 位随机串。

### 13.2 退出登录

- header 增加“退出登录”按钮；`POST /logout` 清会话、下发过期 Cookie（`session=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict`）。
- 旧 Cookie 退出后立即 401；已实测。

### 13.3 过程中的故障与修复

第一次在 `.204` 上执行 `passwd_hash` 时，配置回读校验用了 8 位循环变量 `a_j`，而迁移后的配置约 260 字节（>255），循环永远结束不了，CPU 卡死。
- 现象：管理面 000，但 `.201` 的 MAC 表仍能看到 `.204` 的 MAC，说明 ASIC 转发正常、只是 8051 卡住；设备无看门狗，只能断电。
- 配置写入在卡死前已完成，断电重启后 `auth.json` 返回 `set:1`，摘要登录正常。
- 修复：回读计数器改为 `uint16_t`；`v0.1.6` 已重新验证 `passwd_hash` 持久化（新 salt 写回、旧会话 401、重启后仍可用 1234 登录）。

### 13.4 .204 验收

- `auth.json` 返回 `set:1` + salt；摘要登录 200，错误摘要无 Cookie；明文 `1234` 已不能再登录。
- `/config` 只有 `passwd_hash`，无 `passwd` 明文；`/cmd_log` 无口令。
- 退出登录 302 + 过期 Cookie，旧 Cookie 401。
- `Host: evil` 421；无 CSRF 头 403、带上 200；未认证 `/cmd` 401。
- 日志：`upgrade/logs/v016-pilot-204-20261112-111624.txt`。

### 13.5 白页修复

第一次刷 `v0.1.6` 后，登录能进 `index.html`，但仪表盘空白。原因是新增的 `hdr_logout_t` 后面少了一个逗号，
`app.js` 解析失败（`SyntaxError: Unexpected identifier 'th_auto'`），静态外壳之外的动态内容都没渲染。
`v0.1.7` 已补上逗号，并用 `node --check output/html_min/app.js` 和线上 `/app.js` 做了语法校验。

### 13.6 待办

- 建议补：串口 `passwd_hash` 的持久化钩子（当前只有 HTTP 路径会写回配置）、把 `/config` 的摘要也脱敏（需要服务端保留逻辑）。
- 轮换 `1234`：摘要已不再明文，但 4 位口令可被离线爆破。

## 14. .201–.203 升级（2026-11-12 11:26–11:36）

`.201`、`.202`、`.203` 已按同一流程迁移到 `v0.1.7`：先写 `passwd_hash` + 保留明文回退，确认摘要登录可用后删除明文，再重启复测。

| 设备 | 版本 | salt | 明文 1234 | 结果 |
|---|---|---|---|---|
| .201 | `v0.1.7` | `b84e0a8d…` | 拒绝 | OK |
| .202 | `v0.1.7` | `534d7e12…` | 拒绝 | OK |
| .203 | `v0.1.7` | `e3028deb…` | 拒绝 | OK |
| .204 | `v0.1.7` | `6d834d01…` | 拒绝 | OK（密码已在 Web UI 改过） |

- 三台的启动配置除 `passwd_hash` 外与升级前逐行一致；VLAN 未变。
- 退出登录、`Host` 421、CSRF 403/200、XFO/frame-ancestors/nosniff 三台均通过；日志 `upgrade/logs/v017-fleet-201-203-20261112-113707.txt`。
- `.201` 升级后 NTP 一度无应答，执行 `ntp off`/`ntp on` 后恢复同步；端口 4 从 Down 变为 2.5G，属物理链路变化，需现场确认。
- `.204` 的 salt 从 `5278…` 变为 `6d83…`，说明 Web UI 改密已生效；`1234` 不再可登录，需用新口令。

## 15. 新上线三台：.104 / .162 / .205（2026-11-12 11:43–11:51）

### 15.1 闪存检查

三台都是 **2 MiB Boya/BoHong `BY25Q16BS`**，`RDID 68 40 15`：

| 设备 | 型号 | 硬件 | JEDEC | 构建 |
|---|---|---|---|---|
| .104 | Hi-K0402WS | PCB-K0402W-V1.0 | `684015` | `MACHINE_PCB_K0402W_V1 FLASH_NO_DIO=1` |
| .162 | Hi-K0402WS | PCB-K0402W-V1.0 | `684015` | 同上 |
| .205 | SFP-J08E-HG1 | PCB-K0801W-V1.0 | `684015` | `MACHINE_PCB_K0801W_V1 FLASH_NO_DIO=1` |

这颗 Boya/BoHong 颗粒**必须**用 `FLASH_NO_DIO=1`（Dual-I/O 会取指失败、复位循环；IG204 上次就是这样变砖的）。三台都确认了 JEDEC 和 2 MB 容量后才刷，镜像里也核对了 `machine_name` 与 `hw_ver` 一致。

### 15.2 迁移结果

| 设备 | 旧端口 | 新端口 | salt | 明文 1234 |
|---|---|---|---|---|
| .104 | 80 | **8000** | `5f3c6ac1…` | 拒绝 |
| .162 | 80 | **8000** | `ff4c770b…` | 拒绝 |
| .205 | 8000 | 8000 | `696596cd…` | 拒绝 |

- 三台原先都是 v0.1.1、无口令；先把 `http port 8000`、`passwd 1234`（回退）、`passwd_hash` 写入配置，再刷 v0.1.7，确认摘要登录后删除明文，重启复测。
- `.104/.162` 原来在 80，升级后统一到 8000；80 已关闭。
- `.104/.162` 首次配置只有 `ip dhcp`，升级后补了 `dns server`/`ntp` 行；三台 NTP 已同步。
- 三台 `Host` 421、CSRF 403/200、退出登录 302 + 旧 Cookie 401、UI 退出按钮均通过；VLAN 均为 mgmt 1。
- 当前链路：`.104` p5=10G；`.162` p3=2.5G、p5=10G；`.205` p4=2.5G、p8=2.5G。
- 日志：`upgrade/logs/v017-new-fleet-final-20261112-115118.txt`。

### 15.3 出厂默认

三台镜像的嵌入默认配置是 `upgrade/config/default-digest.txt`：`ip dhcp` + 实验室 DNS/NTP + `http port 8000` + `passwd_hash`（固定 salt/hash，对应口令 `1234`）。恢复出厂后 Web 仍在 8000、口令 `1234`（摘要），不会因为 fail-closed 锁死；该默认摘要三台共用，正式环境应改密。
