# RTLPlayground —— 机型支持分支（fork）

[English](README.en.md) · 上游项目：[logicog/RTLPlayground](https://github.com/logicog/RTLPlayground)

这是 [logicog/RTLPlayground](https://github.com/logicog/RTLPlayground) 的一个分支。在保留上游固件的基础上，它补充了上游目前还没有的硬件支持：

- **5 个新的机型 target**（上游不支持的板子，或上游写错型号的板子）
- **这些板子必需的修复**：RTL8372 芯片上原本完全失效的复位键、同一颗芯片上登录几秒后就失效的 Web 会话、以及被固件遗忘的出厂 MAC
- **它们依赖的新功能**：没有复位键也能恢复出厂设置（Web 界面按钮 + 控制台命令）
- **给下一台机器做适配用的文档和工具**：带板卡照片和实测数据的机型文档、串口抓取脚本、复位键探针

本页所有结论都是**在真机上实测得到的**（6 台设备，2026-10-11 … 2026-10-13）。文中"已验证 / verified"一律表示"在该板子上量过"，机型文档里会写出所用的命令和输出。

| | |
|---|---|
| 固件版本 | `v0.1.0`，并盖上所用提交号（在机器上执行 `version` 可见）；本文实测所用的构建为 `v0.1.0-6a8cd51`。干净的工作区构建出来不带 `-dirty` 后缀 |
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
5. [验证情况](#5-验证情况) —— 6 台实测设备，以及怎么验证你自己的板子
6. [把改动回馈上游](#6-把改动回馈上游)

## 1. 我该编译哪个 target？

### 1.1 本分支新增的 5 个 target

| 板卡丝印 | 零售型号 | 网管 | 端口 | 主控 | 闪存 | Machine target | 编译 |
|---|---|---|---|---|---|---|---|
| `PCB-K0402W-V1.0` / `DIP-K0402WS-V1.0` | 海思创 Hi-K0402WS（众通源） | 非网管 | 4 × 2.5G + 2 × SFP+ | RTL8372 | 2 MiB Boya/BoHong `BY25Q16BS` | `PCB_K0402W_V1` | **必须加 `FLASH_NO_DIO=1`** |
| `PCB-K0801W-V1.0` / `DIP-K0801WS-V1.0` | 海思创 Hi-K0801WS（实物标签 `SFP-J08E-HG1`） | 非网管 | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Boya/BoHong | `PCB_K0801W_V1` | **必须加 `FLASH_NO_DIO=1`**，实验性 |
| `PCB-SWTG115AS-V2.1-17443` | 兮克 XikeStor **SKS3200M-5GPY1XF** | 网管 | 5 × 2.5G + 1 × 万兆 SFP+ | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_5GPY1XF` | `make MACHINE=…` |
| `SWTG024AS-v1.0` | 兮克 XikeStor **SKS3200M-4GPY2XF** | 网管 | 4 × 2.5G + 2 × SFP | RTL8372 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_4GPY2XF` | `make MACHINE=…` |
| `SWTG118AS-V2.1-17462` | 兮克 XikeStor **SKS3200M-8GPY1XF** | 网管 | 8 × 2.5G + 1 × SFP+ | RTL8373 | 2 MiB Winbond `W25Q16JV` | `SKS3200M_8GPY1XF` | `make MACHINE=…` |

三个兮克机型**不是新板子**：里面的 PCB 上游本来就支持（分别是 ZX-SWTGW215AS、SWTG024AS 系列、ZX-SWTGW218AS）。新增 target 的原因是**同一块板子被厂商以另一个型号销售**，固件里应该显示实际型号；另外 4GPY2XF 那台还需要为这个零售版本单独打开复位键和出厂 MAC。

### 1.2 本分支修正的 3 处上游机型信息

| 上游写的是 | 实物其实是 | 影响 |
|---|---|---|
| `XikeStor SKS3200-8E1X`（SWTG118AS-V2.1-17462） | **`SKS3200M-8GPY1XF`** —— 机壳标签和机器自己的 Web 界面都这么写 | 按上游名字去找会找不到；固件也会报一个实物上根本没有的型号 |
| 海思创 Hi-K0801WS 的板卡是 `PCB-KO801W-V2.0` | **`PCB-K0801W-V2.0`** —— 丝印上是数字 `0`，不是字母 `O` | target 和文档已改名，代码树里不再有旧拼写 |
| 海思创 Hi-K0402WS 的 `PCB-K0402W-V1.0`：未收录 | 已收录为**正式支持**，闪存 2 MiB | 原先整个板卡版本缺失 |

同时还修正了：品牌拼写 `Xikestore` → `XikeStor`、4GPY2XF 的闪存容量（`?` → 2 MiB）、8GPY1XF 一行的闪存容量与 PCB 标签。

### 1.3 三步确认你的板子

1. **拆机，看丝印版本号** —— 板边会印 `PCB-…` 字样，如 `PCB-K0402W-V1.0`、`PCB-SWTG115AS-V2.1-17443`、`SWTG024AS-v1.0`；旁边配套的 `DIP-…` 是同一版本的另一半板子。
2. **看机壳标签**：型号名 + 端口数量。它只能告诉你买的是哪个零售版本，**不能**告诉你 PCB 版本 —— 同样是 Hi-K0402WS，就可能落在 V1.0 / V2.0 / V3.0 三种需要**不同 target** 的板子上。
3. **以丝印为准，不要以零售名为准。** 丝印能对上下表就用对应 target；对不上就去 [doc/supported_devices.md](doc/supported_devices.md) 里查 —— 其它版本（KP-9000、SWTG024AS-V2.0、ZX-SWTGW215AS、ZX-SWTGW218AS、PCB-K0402WS V2.0/V3.0 等）上游已经覆盖。

> [!IMPORTANT]
> 上面 5 个 target 里有两个，如果**不加 `FLASH_NO_DIO=1`** 编译，会出现"`Detecting CPU` 之后每 10 ms 重启一次"的死循环。这不是板子坏了，也不是 target 选错了，而是闪存芯片不接受 Dual-I/O 读模式：重新加 `FLASH_NO_DIO=1` 编译再刷即可。

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

### 2.2 海思创 Hi-K0801WS —— `PCB-K0801W-V1.0` → `MACHINE_PCB_K0801W_V1`（实验性）

RTL8373，8 × 2.5GBASE-T + 1 × SFP+，2 MiB Boya/BoHong 闪存。**状态：能启动，但未完全验证 —— 因此刻意*不*列入正式支持清单。**

| | |
|---|---|
| 编译 | `make MACHINE=PCB_K0801W_V1 FLASH_NO_DIO=1` |
| 首次刷机 | 同样只能用 SPI 夹子（原厂固件与上面同款非网管固件）；务必备份 2 MiB |
| 复位键 | **没有**，也没有预留焊盘 |
| 默认 IP | `192.168.10.247` |
| 机型文档 | [doc/devices/PCB-K0801W-V1.0.md](doc/devices/PCB-K0801W-V1.0.md) |

已验证：这块 V1.0 丝印能正常启动、判定为 RTL8373 类（无 `INCORRECT MACHINE!`）、Web 界面 9 个端口位置正确、闪存模式正确。**未验证**：LED 组、LED 焊盘映射、系统灯和 SFP 引脚目前仍是 V2.0 的值 —— 所以要做好"灯不对"的心理准备（不亮、反相或串位），SFP 也可能识别不到模块。文档里列出了逐项确认所需的控制台命令；照片里能看到版本号、日期码和闪存芯片。

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

`make -C test`（主机侧单测，已扩展到覆盖新的型号行）和 `make machine_check` 全绿；CI 会编译两种芯片类别，并额外编译带 health 插桩的版本。文档新增/修正：上面 3 篇新机型文档，加 [doc/devices/SWTGW218AS.md](doc/devices/SWTGW218AS.md)、[doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md)；5 个机型的板卡照片；[doc/supported_devices.md](doc/supported_devices.md)；[doc/hardware.md](doc/hardware.md) 里补了 RTL8372/RTL8373 秒寄存器差异；以及"升级会保留启动配置"的说明（见 4.3）。

### 3.11 SWTG024AS 系列的两个光口编号交叉 —— 修复，影响整个系列

板子丝印的端口总览里，左笼 `J4`（插孔 5）是 **10GMAC 8 / SerDes 1**，右笼 `J2`（插孔 6）是 **10GMAC 3 / SerDes 0** —— 也就是两个光口在逻辑编号上是交叉的。上游的 machine 表把它们写反了（插孔 5 记成逻辑口 3、插孔 6 记成逻辑口 8），于是插在左笼的模块会被报成右笼（灯也亮错），而插在右笼的模块根本不被识别。现在 `log_to_phys_port` / `phys_to_log_port` 与端口总览一致，两个 SFP 槽的 SerDes、检测/LOS 引脚和指示灯也都跟着各自的插孔走。这是板子的属性，所以**上游支持的 SWTG024AS-V2.0 系列（联果 / Horaco / Sodola）也一起修好了**。

### 3.12 SKS3200M-4GPY2XF 的 LED 焊盘映射与两个光笼 —— 修复

该机原来沿用 SWTG024AS-V2.0 的 LED 表，而那台机器的灯在别的焊盘上，导致：端口与颜色全部串位（1000M 链路点亮了右边的绿灯即 2.5G 灯，LAN3 点亮 LAN4 的灯）、两个光口灯完全不亮、还有几个橙灯在没有链路时自己发亮。现在这个 target 带的是**在真机上一个焊盘一个焊盘量出来的映射**（绿/橙 = 12/14、15/17、18/20、21/23，光口灯 = 焊盘 24 和 9），光口灯任何速率都会亮，未使用信号对应的焊盘被驱动到非激活电平而不是悬空（悬空时，凡是被下拉的灯都会亮）。测量方法写进了 [doc/devices/SWTG024AS.md](doc/devices/SWTG024AS.md) 的 "LEDs" 一节。同一台机器的两个光笼也补齐了，见 [2.4](#24-兮克-xikestor-sks3200m-4gpy2xf--swtg024as-v10--machine_sks3200m_4gpy2xf)。

### 3.13 浏览器标签页名 —— 修复

上游让面板标签页显示交换机 hostname，但只在 `/information.json` 已经返回 hostname 之后才生效：页面加载中、请求失败、或者镜像本身没有 hostname 时，几个同时打开的交换机就都变回 "Switch"。现在登录页在首次绘制前就用本机记住的名字命名标签页（每台交换机是独立 origin），面板则按"实时 hostname → 记住的名字 → 型号 → IP"依次回退，并同时写到导航栏。

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
```

产物在 `output/<target>/rtlplayground-<版本>-<target>.bin`（512 KiB）。

### 4.2 刷机

| 机器当前运行 | 用什么方式 |
|---|---|
| RTLPlayground | Web 界面"固件升级"，上传 512 KiB 的 `output/<target>/rtlplayground-*.bin` |
| 原厂**网管**固件（三个兮克机型） | 一键安装镜像 `installer/output/rtlplayground_oem_upgrade.bin`，从原厂"固件升级"页上传 —— loader 的差异见第 2 节各机型说明 |
| 原厂**非网管**固件（Hi-K0402WS、Hi-K0801WS） | SPI 夹子 + 编程器：先备份 2 MiB，擦除，写入填充到 2 MiB 的镜像，回读校验。填充只对编程器有意义，**不要**上传填充过的文件 |

### 4.3 刷完之后

- 刚刷好的兮克机器在 `192.168.10.12`，Hi-K0402WS / Hi-K0801WS 在 `192.168.10.247`；默认密码是项目默认密码。
- **升级会保留启动配置。** 镜像只写到配置区之前就停下，所以 VLAN、端口名、密码、IP 地址都能跨版本保留。镜像内嵌的配置只在"机器还没有配置"（全新刷机）或"恢复出厂设置"之后生效。
- 恢复内嵌配置有三种方式：System 标签页的 *Restore factory defaults*、控制台 `defaults` 命令、以及有按键的机型长按复位键 > 10 s。恢复后机器会换到镜像的默认地址，需要重新连过去。
- 启动日志和 System 标签页会把**板卡版本**和**零售型号**分两行显示，随时能确认这台机器跑的是哪个 target。

## 5. 验证情况

### 5.1 实测过的 6 台设备

| 设备 | Target | 实测内容 |
|---|---|---|
| 海思创 Hi-K0402WS 第 1 台 | `PCB_K0402W_V1` | 完整适配：端口、铜口 LED、左右两个光笼、恢复出厂设置、会话 |
| 海思创 Hi-K0402WS 第 2 台 | `PCB_K0402W_V1` | 界面恢复出厂设置、会话 |
| 海思创 Hi-K0801WS（`SFP-J08E-HG1`） | `PCB_K0801W_V1` | 能启动、9 个端口正确、闪存模式 —— LED 与 SFP 引脚待测 |
| 兮克 SKS3200M-8GPY1XF | `SKS3200M_8GPY1XF` | 复位键三态、会话、恢复出厂设置 |
| 兮克 SKS3200M-4GPY2XF | `SKS3200M_4GPY2XF` | 复位键三态、出厂 MAC、恢复出厂设置；**两个光笼**（插孔 5/6 = 逻辑口 8/3）、**逐焊盘实测的 LED 映射**、端口编号交叉修正（2026-10-13） |
| 兮克 SKS3200M-5GPY1XF | `SKS3200M_5GPY1XF` | 六个端口、光口 10G、LED、系统灯、出厂 MAC、复位键三态 |

6 台都刷了同一份构建、上报相同的软件版本，并且各自的 IP、hostname、出厂 MAC 都原样保留。

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

## 6. 把改动回馈上游

这些改动是叠在上游 `03311de` 之上的一系列自成体系的提交（一个机型 / 一个修复 / 一篇文档），既可以整体提，也可以拆开单独提。整体提交可以从 `jzhou404gmail:main` 发起 PR：
<https://github.com/logicog/RTLPlayground/compare/main...jzhou404gmail:RTLPlayground:main?expand=1>

上游 README 的通用说明完整保留在 [README.en.md](README.en.md#upstream-readme-unchanged)（英文原文），包括：编译环境与 Docker 用法、两种编译目标（直接刷芯片 / 原厂固件升级镜像）、Web 界面安装、直接刷 ROM 的硬件方式、串口连接、Web 界面说明、命令行说明、高级配置，以及全部功能文档的索引。本页只讲**本分支新增/修正的部分**：机型怎么选、这几块板子怎么刷、以及它们特有的注意事项。

上游项目本身的文档（与机型无关的通用功能）都在 [README.en.md](README.en.md) 里，也可以直接看上游仓库：[logicog/RTLPlayground](https://github.com/logicog/RTLPlayground)。
