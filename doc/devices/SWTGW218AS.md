### ZX-SWTGW218AS

## Brands
|Brand|Type|Managed|PCB|Flash|Chip RTL|
|---|---|---|---|---|---|
| Mokerlink | ZX-SWTGW218AS | Yes|  SWTG118AS-V2.0-16029 | 2MB (FM25Q16A)| 8273N + 8224N |
| Sodola | | | | | |
| Horaco | | | | | |
| XikeStor | SKS3200-8E1X | Yes | SWTG118AS-V2.1-17462 | 2MB (25Q16JVSIQ) | |

The XikeStor unit is also sold as `SKS3200M-8GPY1XF`.

## RTLPlayground targets

Both PCB revisions take the same machine parameters, so they share one machine
block. They differ in what they report as the board, which the web UI shows as
硬件 (hardware) and 型号 (model):

| Target | 硬件 (hardware) | 型号 (model) | Use for |
|---|---|---|---|
| `MACHINE_SWTGW218AS` | `SWTGW218AS 8+1 Managed Switch` | – | the SWTG118AS-V2.0-16029 boards (Mokerlink ZX-SWTGW218AS) |
| `MACHINE_SKS3200M_8GPY1XF` | `PCB-SWTG118AS-V2.1` | `SKS3200M-8GPY1XF` | the XikeStor unit on the SWTG118AS-V2.1-17462 PCB |

The two targets are interchangeable as far as the hardware is concerned; pick the
one that matches the silkscreen of your board so the UI states it correctly.

## Photos

<img src="photos/SWTGW218AS-managed/front.png" width="800" />

<img src="photos/SWTGW218AS-managed/label.jpg" width="800" />

## PCB

See up'n'atom's Repo:

https://github.com/up-n-atom/SWTG118AS/blob/main/photos/SWTG118AS-v2.0/SWTG118AS-v2.0-pcb-top.JPG

https://github.com/up-n-atom/SWTG118AS/blob/main/photos/SWTG118AS-v2.0/SWTG118AS-v2.0-pcb-bottom.JPG

