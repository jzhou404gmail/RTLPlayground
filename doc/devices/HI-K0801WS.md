# Hisource Hi-K0801WS

Following is documentation for unmanaged switch marked as `Hi-K0801WS`.

Using SPI clamp in-board is the only method for initial installation.

### Label specifications

- **Name**: 2.5G Ethernet Switch  
- **Model**: Hi-K0801WS  
- **Ports**:  
  - 8 × RJ45: 10/100/1000/2500 Mbps  
  - 1 × SFP: 1000 / 2500 / 10000 Mbps  

### What works (expected from label + similar devices)

- All eight 2.5GBASE-T RJ45 ports at 10/100/1000/2500 Mbps  
- SFP port supporting 1G, 2.5G and 10G modules 
- LEDs

### PCB overview

**Board markings**  
- Top silkscreen: PCB-K0801W-V2.0 / DIP-K0801WS-V2.0  

Top side

<img src="photos/K0801W-V2.0-unmanaged\PCB-top.jpg" width="300" />

Bottom

<img src="photos/K0801W-V2.0-unmanaged\PCB-bottom.jpg" width="300" />

An older PCB revision of the same 8 + 1 model exists, marked `PCB-K0801W-V1.0`. It is
documented separately, with its own target derived from this V2.0 one (its 2 MiB
Boya/BoHong flash needs `FLASH_NO_DIO=1`); its LED and SFP wiring are the values of this
revision: [PCB-K0801W-V1.0](PCB-K0801W-V1.0.md).

## Power supply

Input power is delivered via barell plug, `12V 1A` adapter was provided.
