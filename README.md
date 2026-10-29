# RTLPlayground —— 机型支持分支（fork）

[English](README.en.md) · 上游项目：[logicog/RTLPlayground](https://github.com/logicog/RTLPlayground)

这是 [logicog/RTLPlayground](https://github.com/logicog/RTLPlayground) 的一个分支。在保留上游固件的基础上，它补充了上游目前还没有的硬件支持：

- **6 个新的机型 target**，外加按实物重写并实测的上游 IG204 target（上游不支持的板子，或上游写错型号、只留占位的板子）
- **这些板子必需的修复**：RTL8372 芯片上原本完全失效的复位键、同一颗芯片上登录几秒后就失效的 Web 会话、以及被固件遗忘的出厂 MAC
- **它们依赖的新功能**：没有复位键也能恢复出厂设置（Web 界面按钮 + 控制台命令）
- **给下一台机器做适配用的文档和工具**：带板卡照片和实测数据的机型文档、串口抓取脚本、复位键探针

本页所有结论都是**在真机上实测得到的**（8 台设备，2026-10-11 … 2026-10-29）。文中"已验证 / verified"一律表示"在该板子上量过"，机型文档里会写出所用的命令和输出。

| | |
|---|---|
| 固件版本 | `v0.1.0`，并盖上所用提交号（在机器上执行 `version` 可见）；本文实测所用的构建为 `v0.1.0-e976582`。干净的工作区构建出来不带 `-dirty` 后缀 |
| 基于上游 | `main` @ [`03311de`](https://github.com/logicog/RTLPlayground/commit/03311de) |
| 许可证 | MIT，与上游一致 —— 见 [LICENSE](LICENSE) |
| 完整机型清单 | [doc/supported_devices.md](doc/supported_devices.md) |
| 上游通用说明 | 完整保留在 [README.en.md](README.en.md#upstream-readme-unchanged)（英文原文） |
| 英文版 | [README.en.md](README.en.md)（本页的英文版 + 上游原文） |

## 目录

1. [我该编译哪个 target？](#1-我该编译哪个-target)
2. [逐机型说明](#2-逐机型说明) —— 编译命令、刷机方式、已验证到什么程度
3. [这个分支还改了什么](#3-这个分支还改了什么)
4. [操作速查](#4-操作速查) —— 编译、刷机、首次启动
5. [验证情况](#5-验证情况) —— 8 台实测设备，以及怎么验证你自己的板子

## 1. 我该编译哪个 target？

### 1.1 本分支新增的 6 个 target（外加按实物重写的 IG204）

| 板卡丝印 | 零售型号 | 网管 | 端口 | 主控 | 闪存 | Machine target | 编译 |
|---|---|---|---|---|---|---|---|
| `PCB-K0402W-V1.0` / `DIP-K0402WS-V1.0` | 海思创 Hi-K0402WS（众通源） | 非网管 | 4 × 2.5G + 2 × SFP+ | RTL8372 | 2 MiB Boya/BoHong `BY25Q16BS` | `PCB_K0402W_V1` | **必须加 `FLASH_NO_DIO=1`** |
| `PCB-K0801W-V1.0` / `DIP-K0801WS-V1.0` | 海思创 Hi-K0801WS（实物标签 `SFP-J08E-HG1`） | 非网管 | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Boya/BoHong `BY25Q16BS` | `PCB_K0801W_V1` | **必须加 `FLASH_NO_DIO=1`** |
| `PCB-SWTG115AS-V2.1-17443` | 兮克 XikeStor **SKS3200M-5GPY1XF** | 网管 | 5 × 2.5G + 1 × 万兆 SFP+ | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_5GPY1XF` | `make MACHINE=…` |
| `SWTG024AS-v1.0` | 兮克 XikeStor **SKS3200M-4GPY2XF** | 网管 | 4 × 2.5G + 2 × SFP | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_4GPY2XF` | `make MACHINE=…` |
| `SWTG118AS-V2.1-17462` | 兮克 XikeStor **SKS3200M-8GPY1XF** | 网管 | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_8GPY1XF` | `make MACHINE=…` |
| `PB-2131` | IG204 4+2（标签 `IG204 V1`，Steamemo 等品牌共用） | 非网管 | 4 × 2.5G + 2 × SFP+ | RTL8372 | 原厂 4 MiB cFeon `EN25QE32A`；实测机换成 1 MiB `MX25L8005M` | `STEAMEMO_IG204_V1` | `make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1` |
| `F7005-2.5GPOE-V1.0` | NPA **AP-7024-2.5G** | 非网管 | 4 × 2.5G + 2 × SFP+ | RTL8372 | 原厂 512 KiB BrightMoon `T25S40`；成品机换成 2 MiB `BY25Q16BS` | `F7005_2_5` | `make MACHINE=F7005_2_5 FLASH_NO_DIO=1` |

三个兮克机型**不是新板子**：里面的 PCB 上游本来就支持（分别是 ZX-SWTGW215AS、SWTG024AS 系列、ZX-SWTGW218AS）。新增 target 的原因是**同一块板子被厂商以另一个型号销售**，固件里应该显示实际型号；另外 4GPY2XF 那台还需要为这个零售版本单独打开复位键和出厂 MAC。

IG204 4+2（`PB-2131`）和 NPA AP-7024-2.5G（`F7005-2.5GPOE-V1.0`）则是**两块新板子**：上游的 `MACHINE_STEAMEMO_IG204_V1` 只是一个没有实测过的占位 target（两个光笼的检测脚、LED 焊盘、面板 VLAN 拨刀都没写对），`F7005_2_5` 上游没有。两块板子都逐项量过，细节见第 2 节各自的说明。

### 1.2 本分支修正的 4 处上游机型信息

| 上游写的是 | 实物其实是 | 影响 |
|---|---|---|
| `XikeStor SKS3200-8E1X`（SWTG118AS-V2.1-17462） | **`SKS3200M-8GPY1XF`** —— 机壳标签和机器自己的 Web 界面都这么写 | 按上游名字去找会找不到；固件也会报一个实物上根本没有的型号 |
| 海思创 Hi-K0801WS 的板卡是 `PCB-KO801W-V2.0` | **`PCB-K0801W-V2.0`** —— 丝印上是数字 `0`，不是字母 `O` | target 和文档已改名，代码树里不再有旧拼写 |
| 海思创 Hi-K0402WS 的 `PCB-K0402W-V1.0`：未收录 | 已收录为**正式支持**，闪存 2 MiB | 原先整个板卡版本缺失 |
| Steamemo IG204 4+2 的板卡名写成 `Steamemo IG204 V1`（一整行） | **硬件 `PB-2131` / 型号 `IG204 V1`** 分两行 —— 同一块 PB-2131 板子被多个品牌贴牌销售 | 上游那个 target 只是占位：两个光笼的检测脚、LED 焊盘没有实测值，面板 VLAN 拨刀则完全没有支持。本分支按实物补全 |

同时还修正了：品牌拼写 `Xikestore` → `XikeStor`、4GPY2XF 的闪存容量（`?` → 2 MiB）、8GPY1XF 一行的闪存容量与 PCB 标签。

### 1.3 三步确认你的板子

1. **拆机，看丝印版本号** —— 板边会印 `PCB-…` 字样，如 `PCB-K0402W-V1.0`、`PCB-SWTG115AS-V2.1-17443`、`SWTG024AS-v1.0`；旁边配套的 `DIP-…` 是同一版本的另一半板子。
2. **看机壳标签**：型号名 + 端口数量。它只能告诉你买的是哪个零售版本，**不能**告诉你 PCB 版本 —— 同样是 Hi-K0402WS，就可能落在 V1.0 / V2.0 / V3.0 三种需要**不同 target** 的板子上。
3. **以丝印为准，不要以零售名为准。** 丝印能对上下表就用对应 target；对不上就去 [doc/supported_devices.md](doc/supported_devices.md) 里查 —— 其它版本（KP-9000、SWTG024AS-V2.0、ZX-SWTGW215AS、ZX-SWTGW218AS、PCB-K0402WS V2.0/V3.0 等）上游已经覆盖。

> [!IMPORTANT]
> 上面这些 target 里有四个是用 `FLASH_NO_DIO=1` 编译并实测的（Hi-K0402WS、Hi-K0801WS、IG204、AP-7024-2.5G）。其中 **Hi-K0402WS / Hi-K0801WS 不加这个标志一定会出问题**：`Detecting CPU` 之后每 10 ms 重启一次，这不是板子坏了，也不是 target 选错了，而是那两颗 Boya `BY25Q16BS` 不接受 Dual-I/O 读模式，重新加 `FLASH_NO_DIO=1` 编译再刷即可。IG204 和 AP-7024 的闪存（`MX25L8005M`、`T25S40`、`BY25Q16BS`）同样只按单比特 fast read 验证过，一起加这个标志更稳。

## 2. 逐机型说明

### 2.1 海思创 Hi-K0402WS —— `PCB-K0402W-V1.0` → `MACHINE_PCB_K0402W_V1`

RTL8372，4 × 2.5GBASE-T + 2 × SFP+（1G/2.5G/10G），单颗 2 MiB Boya/BoHong `BY25Q16BS`（`RDID 68 40 15`），主控在散热片下。**状态：正式支持，两台实测。**

| | |
|---|---|
| 编译 | `make MACHINE=PCB_K0402W_V1 FLASH_NO_DIO=1` |
| 首次刷机 | 只能用 SPI 夹子。原厂是非网管的 Realtek 参考固件，**9600 8N1** 的工厂测试串口，没有 Web 界面，所以第一次装必须上编程器 |
| 编程器镜像 | 把 512 KiB 的构建填充到 2 MiB（`truncate -s 2097152 …`）；**填充过的镜像绝对不能**用于 Web 升级 |
| 复位键 | **没有**，连未焊的焊盘都没有 → 用 Web 界面按钮或 `defaults` 命令恢复内嵌配置 |
| 默认 IP | `192.168.10.247`（来自 `config.txt`） |
| 串口 | 跑起 RTLPlayground 后为 115200 8N1 |
| 机型文档 | [doc/devices/PCB-K0402W-V1.0.md](doc/devices/PCB-K0402W-V1.0.md) |

实测内容：加 `FLASH_NO_DIO=1` 后正常启动；`regget 0004` = `0x83720000`（RTL8372）；六个端口顺序正确（逐口插线验证）；铜口 LED（2.5G 亮橙、1G/100M/10M 亮绿并随流量闪，含真实 2.5G 对端）；**左右两个 SFP 笼**都验过（模块检测脚、RX_LOS、I2C 读 EEPROM、10G 链路）；Web 界面的"恢复出厂设置"可用。

### 2.2 海思创 Hi-K0801WS —— `PCB-K0801W-V1.0` → `MACHINE_PCB_K0801W_V1`

RTL8373，8 × 2.5GBASE-T + 1 × SFP+，2 MiB Boya/BoHong `BY25Q16BS`（`RDID 68 40 15`）。**状态：支持（2026-10-12 首次适配并实测）。**

| | |
|---|---|
| 编译 | `make MACHINE=PCB_K0801W_V1 FLASH_NO_DIO=1` |
| 首次刷机 | 同样只能用 SPI 夹子（原厂固件与上面同款非网管固件）；务必备份 2 MiB |
| 复位键 | **没有**，也没有预留焊盘 |
| 默认 IP | `192.168.10.247` |
| 机型文档 | [doc/devices/PCB-K0801W-V1.0.md](doc/devices/PCB-K0801W-V1.0.md) |

已验证：这块 V1.0 丝印能正常启动、判定为 RTL8373 类（无 `INCORRECT MACHINE!`）、Web 界面 9 个端口位置正确、闪存模式正确（必须 `FLASH_NO_DIO=1`）。LED 组、LED 焊盘映射、系统灯和 SFP 引脚沿用 V2.0 的值，没有在这块板子上单独量过；文档里列出了逐项确认所需的控制台命令，照片里能看到版本号、日期码和闪存芯片。

### 2.3 兮克 XikeStor SKS3200M-5GPY1XF —— `PCB-SWTG115AS-V2.1-17443` → `MACHINE_SKS3200M_5GPY1XF`

RTL8372，5 × 2.5GBASE-T + 1 × 万兆 SFP+（第 5 个电口由外置 RTL8221B 驱动），2 MiB Winbond `W25Q16JV`（`EF 40 15`）。**状态：正式支持，六个端口全部实测。** 与联果 ZX-SWTGW215AS 是同一块 PCB，两个 target 只差在向界面报告的板卡/型号名。

| | |
|---|---|
| 编译 | `make MACHINE=SKS3200M_5GPY1XF`（或 `MACHINE=LIANGUO_ZX_SWTGW215AS`，接线相同） |
| 刷机 | 先完整备份 2 MiB（原厂镜像无法从 dump 复现）。还在原厂固件：用一键安装镜像；已经跑 RTLPlayground：Web 界面"固件升级"直接上传 |
| 复位键 | `GPIO54` —— **实测三态**：< 3 s 无动作，约 5 s 重启（保留启动配置），约 13 s 恢复出厂设置 |
| 默认 IP | `192.168.10.12`（兮克 target 内嵌 `config-xikestor.txt`） |
| 机型文档 | [doc/devices/SKS3200M-5GPY1XF.md](doc/devices/SKS3200M-5GPY1XF.md) |

实测内容：端口顺序（界面 1-5 = 网口 1-5，界面 6 = 光口）、1G 与 2.5G 链路、光口模块检测/RX_LOS/EEPROM 读取与 10G 链路、电口 LED（2.5G 亮右绿、1G/100M 亮左橙）、系统灯、`0x1FC000` 处的出厂 MAC，以及上面的复位键三态。

### 2.4 兮克 XikeStor SKS3200M-4GPY2XF —— `SWTG024AS-v1.0` → `MACHINE_SKS3200M_4GPY2XF`

RTL8372，4 × 2.5GBASE-T + 2 × SFP，2 MiB Winbond `W25Q16JV`。**状态：正式支持（2026-10-13 完成 LED、双光口实测）。** 与 SWTG024AS-V2.0 系列同族，但**两个 target 不可互换**：这个 target 有实测的 LED 焊盘映射、描述了两个光笼，并且为它单独打开了复位键和出厂 MAC。

| | |
|---|---|
| 编译 | `make MACHINE=SKS3200M_4GPY2XF` |
| 原厂 → RTLPlayground | 原厂是 CGI 版 Realtek SDK（固件版本 V10.9.4，`admin/admin`，`192.168.10.12`）。它的"固件升级"页（`fwug.cgi`）会把机器切进厂商 loader，**IP 不变**；loader 再用 `/httpupg.cgi?cmd=fw_upgrade` 接收一键安装镜像 |
| 复位键 | `GPIO54`（K1）—— **实测**：约 5 s 重启，约 11 s 恢复出厂设置，< 3 s 无动作 |
| 出厂 MAC | 已找回：`mac_flash_offset = 0x1FC000` 让机器重新报告 `1C:2A:A3:23:E4:5E`，不再是随机生成的 MAC |
| 两个光口 | `J4`（左，插孔 5）= 逻辑口 **8**（SerDes 1，检测 GPIO30 / LOS GPIO37，I2C SDA4+SCL3）；`J2`（右，插孔 6）= 逻辑口 **3**（SerDes 0，检测 GPIO50 / LOS GPIO51，I2C SDA3+SCL3）。两个光口在逻辑编号上是**交叉**的 |
| LED | 本机实测的焊盘映射：绿/橙分别在焊盘 12/14（口 1）、15/17（口 2）、18/20（口 3）、21/23（口 4），两个光口灯在焊盘 24（J4）和 9（J2）；光口灯任何速率都会亮 |
| 默认 IP | `192.168.10.12` |
| 机型文档 | [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md) |

> [!NOTE]
> 如果你的路由器/软路由有基于 MAC 的 DHCP 保留或绑定，刷完这个 target 后请按出厂 MAC 重新设置一次 —— 之前那个随机 MAC 不会再回来。
>
> 光口编号的交叉是**板子本身**的属性，所以同一个修复也让上游支持的 SWTG024AS-V2.0（联果/ Horaco / Sodola）受益：在这之前，插在左笼（J4）的模块会被报成右笼。

### 2.5 兮克 XikeStor SKS3200M-8GPY1XF —— `SWTG118AS-V2.1-17462` → `MACHINE_SKS3200M_8GPY1XF`

RTL8373，8 × 2.5GBASE-T + 1 × SFP+，2 MiB Winbond `W25Q16JV`。**状态：正式支持。** 与 Mokerlink ZX-SWTGW218AS（V2.0）同板；这个 target 的存在是为了让界面报告板卡 `PCB-SWTG118AS-V2.1`、型号 `SKS3200M-8GPY1XF`。

| | |
|---|---|
| 编译 | `make MACHINE=SKS3200M_8GPY1XF`（V2.0 板子用 `MACHINE=SWTGW218AS`） |
| 原厂 → RTLPlayground | 原厂是 CGI 固件（固件版本 V1.9，`admin/admin`，`192.168.10.12`），其"固件升级"页（`fwug.cgi`）可直接接收一键安装镜像 |
| 复位键 | `GPIO54` —— **实测**：约 5 s 重启，约 13 s 恢复出厂设置，< 3 s 无动作 |
| 默认 IP | `192.168.10.12` |
| 机型文档 | [doc/devices/SWTGW218AS.md](doc/devices/SWTGW218AS.md) |

实测内容：按键三态、会话处理、Web 界面恢复出厂设置。

### 2.6 IG204 4+2 —— `PB-2131`（标签 `IG204 V1`）→ `MACHINE_STEAMEMO_IG204_V1`

RTL8372，4 × 2.5GBASE-T + 2 × SFP+，原厂 4 MiB cFeon `EN25QE32A-104HIP`（SOP-8，`U5`），前面板带一个 **VLAN ON/OFF 拨刀**。**状态：正式支持（2026-10-29 完成端口、LED、两个光笼和拨刀实测）。** 上游同一个 target 只是个占位：光口检测脚、LED 焊盘、面板拨刀都没有实测值，本分支按实物重写；界面按约定显示 **硬件 `PB-2131` / 型号 `IG204 V1`** 两行。

| | |
|---|---|
| 编译 | `make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1` |
| 首次刷机 | 只能用 SPI 夹子/编程器：原厂是非网管的 Realtek 参考固件（串口 **9600 8N1**，提示符 `RTL8371B:`，没有 Web 界面）。实测机把原厂 4 MiB 闪存换成了 1 MiB `MX25L8005M`（512 KiB 镜像 + Web 升级需要 `flash_size >= 1 MB`） |
| 已经跑 RTLPlayground | Web 界面"固件升级"上传 512 KiB 的 `output/STEAMEMO_IG204_V1/rtlplayground-*.bin` 即可 |
| 复位键 | **没有**（连焊盘都没有）→ Web 界面 *Restore factory defaults* 或控制台 `defaults` |
| 默认 IP | `192.168.10.247`（`config.txt`） |
| 串口 | 控制台排针 `T7`（TX/GND/RX/3V3），跑起 RTLPlayground 后 115200 8N1 |
| 面板 VLAN 拨刀 | `GPIO36`，ON 读 1 / OFF 读 0；ON 时四个电口互相隔离、只留 SFP+ 和 CPU 口可达，OFF 全通。每秒轮询，约 1 秒生效 |
| 机型文档 | [doc/devices/STEAMEMO_IG204_V1.md](doc/devices/STEAMEMO_IG204_V1.md) |

实测内容：`regget 0004` = `0x83720000`（RTL8372，无外置 PHY）；四个电口 2.5G；电口 LED **2.5G 橙 / 1G·100M·10M 绿**、两种颜色**都随流量闪烁**；**左右两个光笼**都验过（检测脚 GPIO30/GPIO50、I²C 读 EEPROM、10G 链路，两只 10G 模块互换插过）；SYS 灯按说明书慢闪；面板 VLAN 拨刀 10 次采样两个位置；随机说明书里的每一条（VLAN 行为、PWR/SYS/LINK-ACT 灯、背板带宽等）逐条对照，写在机型文档的 "Conformance with the manual" 一节。

### 2.7 NPA AP-7024-2.5G —— `F7005-2.5GPOE-V1.0` → `MACHINE_F7005_2_5`

RTL8372，4 × 2.5GBASE-T + 2 × SFP+（1G/2.5G/10G），原厂 512 KiB BrightMoon `T25S40`，12 V 1 A 圆口供电，没有复位键（也没有焊盘）。**状态：支持（2026-10-19 实测电口、LED 和两个光笼的 10G）。** 上游没有这个 target。

| | |
|---|---|
| 编译 | `make MACHINE=F7005_2_5 FLASH_NO_DIO=1` |
| 首次刷机 | 512 KiB 的 `T25S40` 用 SPI 夹子/编程器写（镜像正好 512 KiB，写在偏移 0） |
| Web 升级 | **512 KiB 的板子不行**：固件把升级镜像暂存在 `0x80000`，要求闪存 ≥ 1 MB，否则上传返回 HTTP 400 `Flash too small for firmware upload!`。把闪存换成 2 MiB `BY25Q16BS`（同 SOP-8 封装）之后 Web 升级正常，两个光笼也会自动识别 |
| 复位键 | **没有**，`.reset_pin = GPIO_NA` → Web 界面按钮或 `defaults` |
| 默认 IP | `192.168.10.247` |
| 串口 | 排针 `JP1`（VCC/RX/GND/TX）；原厂 9600 8N1，RTLPlayground 115200 8N1 |
| 机型文档 | [doc/devices/F7005-2.5GPOE-V1.0.md](doc/devices/F7005-2.5GPOE-V1.0.md) |

实测内容：`regget 0004` = `0x83727000`（RTL8372）；四个电口 2.5G；电口 LED **2.5G 绿 / 1G·100M 橙**，逐个焊盘量出来的映射（RJ45 1 绿 3 / 橙 4-5，RJ45 2 绿 6 / 橙 8，RJ45 3 绿 9-10 / 橙 12-14，RJ45 4 绿 18-20 / 橙 21-23）；`LED_GLB_IO_EN` 必须写 `0x7fffffff`，否则整块面板不亮；两个光笼的检测脚是 **GPIO38（左）/ GPIO37（右）**、EEPROM 在 **SDA1/SCL1 与 SDA0/SCL0**（都不是参考值），同一只 10G 模块左右互换都验过 `sfp` 和 10G 链路。

## 3. 这个分支还改了什么

### 3.1 没有复位键也能恢复出厂设置 —— 新功能

System 标签页（*维护*）新增 **Restore factory defaults** 按钮，控制台新增等价的 `defaults` 命令。二者都会用镜像内嵌的配置覆盖启动配置并重启，之后机器会回到镜像的默认地址（兮克构建为 `192.168.10.12`，其它构建为 `config.txt` 里的地址）。

它**刻意不是**一条配置命令：永远不会被写进启动配置。界面文案已补齐 6 种语言（英、日、中、西、法、德）。

为什么重要：Hi-K0402WS 和 Hi-K0801WS 根本没有复位键，在这个功能之前，这两块板子没有任何办法恢复内嵌配置。已在两台 Hi-K0402WS 以及 4GPY2XF、5GPY1XF、8GPY1XF 上实测。

### 3.2 复位键计时 —— 修复，影响所有带按键的 RTL8372 机型

原 `handle_button()` 用 `RTL837X_REG_SEC_COUNTER` 计时。该寄存器只有 RTL8373 才存得住值，RTL8372 上**永远读回 0**，于是每次按键都被算作"短按"，**按键完全失效**。现在改用软件 tick 计数（用挂钟实测 200.0 ticks/s，所以阈值就是真实的秒）：

| 按住时长 | 行为 |
|---|---|
| < 3 s | 无动作 |
| > 3 s | 重启，保留启动配置 |
| > 10 s | 用镜像内嵌配置覆盖启动配置，然后重启 |

三态已在 SKS3200M-4GPY2XF / -5GPY1XF / -8GPY1XF 上实测。这是共享代码，所以**所有**带按键的 RTL8372 机型都跟着受益，包括本分支没动过的机型。

### 3.3 Web 会话超时 —— 修复，RTL8372

HTTP 会话的截止时间同样来自那个秒寄存器，导致 RTL8372 上登录几秒后 cookie 就不再被接受。现在改用 tick 计数，两种芯片上都正确（各芯片分别实测过）。

### 3.4 出厂 MAC —— 修复，SKS3200M-4GPY2XF

该机原来上报随机生成的本地管理 MAC，而不是原厂固件显示的 MAC。为这个 target 单独设置 `.mac_flash_offset = 0x1FC000`（只对它开，因为 V2.0 兄弟版那块区域未验证）之后，出厂 MAC 回来了：`/information.json` 重新显示 `1c:2a:a3:23:e4:5e`，hostname 也随之恢复。

### 3.5 板卡与型号分两行显示 —— 新功能

`struct machine` 新增 `model_name` 字段。target 可以同时填 PCB 版本（`machine_name`，界面显示 硬件 / hardware）和零售型号（`model_name`，显示 型号 / model），这样"丝印与销售型号不一致"的板子不会再被误认成别的机器。不填 `model_name` 的 target 行为与以前完全一致。

### 3.6 每个 target 自己的出厂默认配置 —— 新功能

三个兮克 target 内嵌的是 `config-xikestor.txt`（IP `192.168.10.12`）而不是 `config.txt`，由 Makefile 按 target 选择（`DEFAULT_CONFIG`）。所以刚刷好的兮克机器是 `192.168.10.12`，而不是 `192.168.10.247`。

### 3.7 `FLASH_NO_DIO=1` —— 新构建选项

部分 SPI NOR（Boya/BoHong `BY25Q16BS` 及同族，`RDID 68 40 15`）在闪存控制器的 Dual-I/O MMIO 读模式下会返回乱码。`FLASH_NO_DIO=1` 改用单比特 fast read：这类芯片正常，而目前见过的所有 3.3 V SPI NOR 也都支持。之所以做成构建选项而不是改默认值，是因为 Dual-I/O 对其它已支持板子是有效的提速优化。

### 3.8 构建体验

没有选择机型时直接 `make`，现在会打印可用 target 列表，而不是抛出一堆难懂的报错；`make machine_check` 会把 `machine.h` 里所有 target 的 `machine.c` 全编一遍（CI 也会跑这两项）。

### 3.9 给下一台机器准备的工具

- [`tools/console_capture.py`](tools/console_capture.py) —— 带时间戳的串口抓取，不需要 pyserial。非网管板子原厂是 9600 8N1、跑 RTLPlayground 后是 115200 8N1，正好用它切换。
- [`tools/reset_button_probe.py`](tools/reset_button_probe.py) —— 通过 HTTP 轮询按键 GPIO 电平和开机时长，从而在"人工按着按键"的情况下区分短按、重启和恢复出厂。具体方法（先在启动配置里写一个 marker hostname）写在 [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md)。

### 3.10 测试与文档

`make -C test`（主机侧单测，已扩展到覆盖新的型号行）和 `make machine_check` 全绿；CI 会编译两种芯片类别，并额外编译带 health 插桩的版本。文档新增/修正：第 2 节的 7 篇机型文档（其中 [STEAMEMO_IG204_V1.md](doc/devices/STEAMEMO_IG204_V1.md) 是按实物重写的，[F7005-2.5GPOE-V1.0.md](doc/devices/F7005-2.5GPOE-V1.0.md) 是新增的），加 [doc/devices/SWTGW218AS.md](doc/devices/SWTGW218AS.md)、[doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md)；7 个机型的板卡照片与说明书照片；[doc/supported_devices.md](doc/supported_devices.md)；[doc/hardware.md](doc/hardware.md) 里补了 RTL8372/RTL8373 秒寄存器差异；以及"升级会保留启动配置"的说明（见 4.3）。

### 3.11 SWTG024AS 系列的两个光口编号交叉 —— 修复，影响整个系列

板子丝印的端口总览里，左笼 `J4`（插孔 5）是 **10GMAC 8 / SerDes 1**，右笼 `J2`（插孔 6）是 **10GMAC 3 / SerDes 0** —— 也就是两个光口在逻辑编号上是交叉的。上游的 machine 表把它们写反了（插孔 5 记成逻辑口 3、插孔 6 记成逻辑口 8），于是插在左笼的模块会被报成右笼（灯也亮错），而插在右笼的模块根本不被识别。现在 `log_to_phys_port` / `phys_to_log_port` 与端口总览一致，两个 SFP 槽的 SerDes、检测/LOS 引脚和指示灯也都跟着各自的插孔走。这是板子的属性，所以**上游支持的 SWTG024AS-V2.0 系列（联果 / Horaco / Sodola）也一起修好了**。

### 3.12 SKS3200M-4GPY2XF 的 LED 焊盘映射与两个光笼 —— 修复

该机原来沿用 SWTG024AS-V2.0 的 LED 表，而那台机器的灯在别的焊盘上，导致：端口与颜色全部串位（1000M 链路点亮了右边的绿灯即 2.5G 灯，LAN3 点亮 LAN4 的灯）、两个光口灯完全不亮、还有几个橙灯在没有链路时自己发亮。现在这个 target 带的是**在真机上一个焊盘一个焊盘量出来的映射**（绿/橙 = 12/14、15/17、18/20、21/23，光口灯 = 焊盘 24 和 9），光口灯任何速率都会亮，未使用信号对应的焊盘被驱动到非激活电平而不是悬空（悬空时，凡是被下拉的灯都会亮）。测量方法写进了 [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md) 的 "LEDs" 一节。同一台机器的两个光笼也补齐了，见 [2.4](#24-兮克-xikestor-sks3200m-4gpy2xf--swtg024as-v10--machine_sks3200m_4gpy2xf)。

### 3.13 浏览器标签页名 —— 修复

上游让面板标签页显示交换机 hostname，但只在 `/information.json` 已经返回 hostname 之后才生效：页面加载中、请求失败、或者镜像本身没有 hostname 时，几个同时打开的交换机就都变回 "Switch"。现在登录页在首次绘制前就用本机记住的名字命名标签页（每台交换机是独立 origin），面板则按"实时 hostname → 记住的名字 → 型号 → IP"依次回退，并同时写到导航栏。

### 3.14 面板 VLAN 拨刀开关 —— 新功能

前面板标着 **VLAN ON/OFF** 的拨刀（IG204 上在 SYS 灯左侧）由固件接管：`vlan_switch_poll()` 每秒采样一次 GPIO，拨到 ON 就把四个电口的隔离掩码写成 `0x308`（彼此隔离，但都能到 SFP+ 和 CPU 口，所以管理地址和上联不会掉），拨到 OFF 写回 `0x3f8`（全通）。只有状态变化时才写寄存器，拨完约 1 秒生效、不用重启。

配套的东西：

- 控制台 `vlan switch [on|off]`：面板拨刀这个**功能本身**的开关，是一条普通配置命令（`on` 是默认值），`vlan switch off` 之后拨刀失效，VLAN 和隔离完全由 Web/控制台的 VLAN、隔离设置决定；要重启后保留就 *Save to flash*。
- 仪表盘右上角的 **「VLAN物理拨刀开关设置」** 卡片：一个只读开关显示拨刀的实际位置（下面一行小字提示这个位置意味着什么），下面一个「启用VLAN面板开关」开关控制功能本身。样式与生成树页面的端口开关一致，卡片宽度与端口条相同，和左边的端口条同一行。
- 互斥锁：只要**「启用VLAN面板开关」是启用**，VLANs、Isolation、端口镜像三个页面就整体变灰只读（黄色提示条说明"由面板拨刀接管"），不管物理拨刀在 ON 还是 OFF；把这个开关关掉，三个页面立刻恢复可编辑。之所以按功能开关而不是按拨刀位置加锁：拨刀位置随时会被人拨动，按位置加锁会让页面在编辑途中突然变成只读。

### 3.15 端口灯随流量闪烁 —— 修复，IG204 / AP-7024-2.5G

这两个 target 的 2.5G 灯一开始只写了 `LEDS_2G5 | LEDS_LINK`，漏了 `LEDS_ACT`，于是**在 2.5G（也就是这两块板子日常工作的速率）上灯是常亮的，传输数据时不闪**，另一种颜色又因为速率不匹配而不亮 —— 现象就是"网口灯一直亮着不动"。现在两种颜色都带 `LEDS_ACT`（和 SWTG024AS、Hi-K0801WS 这些已经验证过的 target 一致），说明书写的就是"闪烁 = 数据传输中"。

在 IG204 上验证的办法：`gpio` 命令的第二个字是"与上一次调用相比变了哪些脚"，边 `ping -f` 边轮询，报文在流动时 RJ45 4 的橙灯焊盘（21）会不停出现在变化位上；把 `LED1_0_SET0` 的 `ACT` 位用 `regset 6548 01740041` 现场清掉，同一个焊盘就完全不动了，写回去又恢复。顺带把 SYS 灯按说明书改成空闲慢闪（`sys_led_blink_idle`）。

## 4. 操作速查

### 4.1 编译

```bash
# Hi-K0402WS（PCB-K0402W-V1.0）与 Hi-K0801WS（PCB-K0801W-V1.0）：Boya 闪存必须加这个标志
make MACHINE=PCB_K0402W_V1 FLASH_NO_DIO=1
make MACHINE=PCB_K0801W_V1 FLASH_NO_DIO=1

# 三个兮克机型
make MACHINE=SKS3200M_4GPY2XF
make MACHINE=SKS3200M_5GPY1XF
make MACHINE=SKS3200M_8GPY1XF

# IG204 4+2（PB-2131）与 NPA AP-7024-2.5G（F7005-2.5GPOE-V1.0）：单比特 fast read 验证过
make MACHINE=STEAMEMO_IG204_V1 FLASH_NO_DIO=1
make MACHINE=F7005_2_5 FLASH_NO_DIO=1
```

产物在 `output/<target>/rtlplayground-<版本>-<target>.bin`（512 KiB）。

### 4.2 刷机

| 机器当前运行 | 用什么方式 |
|---|---|
| RTLPlayground | Web 界面"固件升级"，上传 512 KiB 的 `output/<target>/rtlplayground-*.bin` |
| 原厂**网管**固件（三个兮克机型） | 一键安装镜像 `installer/output/rtlplayground_oem_upgrade.bin`，从原厂"固件升级"页上传 —— loader 的差异见第 2 节各机型说明 |
| 原厂**非网管**固件（Hi-K0402WS、Hi-K0801WS） | SPI 夹子 + 编程器：先备份 2 MiB，擦除，写入填充到 2 MiB 的镜像，回读校验。填充只对编程器有意义，**不要**上传填充过的文件 |
| 原厂**非网管**固件（IG204 4+2） | SPI 夹子 + 编程器：先备份原厂 4 MiB `EN25QE32A`，512 KiB 镜像写在偏移 0；原厂芯片没和 RTLPlayground 一起试过（实测机换成了 1 MiB `MX25L8005M`） |
| 原厂**非网管**固件（NPA AP-7024-2.5G） | 512 KiB `T25S40`：SPI 编程器写 512 KiB 镜像（不用填充）。想用 Web 升级就先换成 2 MiB `BY25Q16BS`（同 SOP-8 封装），见 [机型文档](doc/devices/F7005-2.5GPOE-V1.0.md) |

### 4.3 刷完之后

- 刚刷好的兮克机器在 `192.168.10.12`，其它机型（Hi-K0402WS / Hi-K0801WS / IG204 4+2 / AP-7024-2.5G）在 `192.168.10.247`；默认密码是 **`1234`**。
- **升级会保留启动配置。** 镜像只写到配置区之前就停下，所以 VLAN、端口名、密码、IP 地址都能跨版本保留。镜像内嵌的配置只在"机器还没有配置"（全新刷机）或"恢复出厂设置"之后生效。
- 恢复内嵌配置有三种方式：System 标签页的 *Restore factory defaults*、控制台 `defaults` 命令、以及有按键的机型长按复位键 > 10 s。恢复后机器会换到镜像的默认地址，需要重新连过去。
- 启动日志和 System 标签页会把**板卡版本**和**零售型号**分两行显示，随时能确认这台机器跑的是哪个 target。
- 面板上标着 **VLAN** 的拨刀（IG204 上在 SYS 灯左侧）现在由固件接管：拨到 ON 时四个 RJ45 口互相隔离，但都还能到两个光口和 CPU 口（管理地址和上联不受影响）；拨到 OFF 恢复全通。每秒轮询，拨完约 1 秒生效、不用重启，Web 界面的 *Isolation* 标签页能看到对应掩码。仪表盘右上角的「VLAN物理拨刀开关设置」卡片是一个只读的拨刀状态 + 一个「启用VLAN面板开关」开关：关掉这个开关，物理拨刀就失效，VLAN 和隔离完全交给 Web 的 VLAN/隔离设置（等价命令 `vlan switch off`，要重启后保留就 *Save to flash*）。
- **别把同一台交换机的多根线接到另一台交换机的多个口上。** STP 默认关闭，这样做会成环：IG204 上同时接 RJ45 1-3 和两个光口时就出现过约 90 万包/秒的广播风暴，管理地址直接失联，拔掉多余网线才恢复。两台交换机之间只连一根线，或者先 `stp on` 再保存配置。

## 5. 验证情况

### 5.1 实测过的 8 台设备

| 设备 | Target | 实测内容 |
|---|---|---|
| 海思创 Hi-K0402WS 第 1 台 | `PCB_K0402W_V1` | 完整适配：端口、铜口 LED、左右两个光笼、恢复出厂设置、会话 |
| 海思创 Hi-K0402WS 第 2 台 | `PCB_K0402W_V1` | 界面恢复出厂设置、会话 |
| 海思创 Hi-K0801WS（`SFP-J08E-HG1`） | `PCB_K0801W_V1` | 能启动、9 个端口正确、闪存模式（`FLASH_NO_DIO=1`）；LED 与 SFP 引脚沿用 V2.0 值，未在该板上单独实测 |
| 兮克 SKS3200M-8GPY1XF | `SKS3200M_8GPY1XF` | 复位键三态、会话、恢复出厂设置 |
| 兮克 SKS3200M-4GPY2XF | `SKS3200M_4GPY2XF` | 复位键三态、出厂 MAC、恢复出厂设置；**两个光笼**（插孔 5/6 = 逻辑口 8/3）、**逐焊盘实测的 LED 映射**、端口编号交叉修正（2026-10-13） |
| 兮克 SKS3200M-5GPY1XF | `SKS3200M_5GPY1XF` | 六个端口、光口 10G、LED、系统灯、出厂 MAC、复位键三态 |
| IG204 4+2（PB-2131，标签 `IG204 V1`） | `STEAMEMO_IG204_V1` | 换 1 MiB MX25L8005M 后启动、Web 升级、四个铜口 2.5G、2.5G 橙 / 千兆绿、**两条灯都随流量闪烁**、**两个光笼 10G**（检测脚 + I²C + SerDes）、**面板 VLAN 拨刀（GPIO36）**、**SYS 慢闪**；界面显示 硬件 `PB-2131` / 型号 `IG204 V1`；**逐条对照随机说明书**；见 [doc/devices/STEAMEMO_IG204_V1.md](doc/devices/STEAMEMO_IG204_V1.md) |
| NPA AP-7024-2.5G（`F7005-2.5GPOE-V1.0`） | `F7005_2_5` | 四个电口 2.5G、**逐焊盘实测的 LED 映射**（2.5G 绿 / 1G·100M 橙）、`LED_GLB_IO_EN` 修正、**两个光笼 10G**（检测脚 GPIO38/GPIO37、EEPROM 走 SDA1/SCL1（左）与 SDA0/SCL0（右），都不是参考引脚）；512 KiB `T25S40` 与 2 MiB `BY25Q16BS` 都能启动，后者才支持 Web 升级；见 [doc/devices/F7005-2.5GPOE-V1.0.md](doc/devices/F7005-2.5GPOE-V1.0.md) |

8 台都刷了同一份构建、上报相同的软件版本，并且各自的 IP、hostname、出厂 MAC 都原样保留。

### 5.2 怎么验证你自己的板子

刷机前，在主机上：

```bash
make -C test          # 主机侧单元测试
make machine_check    # machine.c 必须对每个 target 都能编译
```

板子跑起来后（控制台或 Web 界面的命令行）：

```
version        # target、板卡版本、零售型号、构建信息
regget 0004    # 第 1 字节：0x73 = RTL8373，其余为 RTL8372
gpio           # 引脚电平 —— 往两个光笼分别插模块，看哪一位翻转
leds           # 导出/强制 LED 组，对照面板验证机型定义
sfp            # 每个光笼的模块存在性与 EEPROM
port <n> show  # 某根网线落在哪个逻辑口
```

验证复位键时，先在启动配置里写一个 marker（例如 `hostname RESETTEST`），再用 [`tools/reset_button_probe.py`](tools/reset_button_probe.py)：重启表现为开机时长归零而 marker 仍在，恢复出厂表现为 marker 消失。
