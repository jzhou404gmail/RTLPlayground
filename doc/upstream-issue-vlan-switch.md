# Front-panel VLAN slide switch: describe the hardware isolation in firmware and UI

Some boards carry a front-panel "VLAN" slide switch next to the SYS LED. On the
IG204 (PCB PB-2131, RTL8372, 4x2.5G + 2xSFP+) it is a two-position switch wired
to GPIO36: it reads 1 in the ON position and 0 in the OFF position. The switch
is not connected to the ASIC; the firmware has to read it and configure port
isolation itself, which is also what the stock firmware does.

This issue proposes a small, board-described feature for it, and asks for the
maintainers' preferred shape before a PR.

## Behaviour

| Panel switch | Meaning |
|---|---|
| ON  | The four RJ45 ports are isolated from each other. Every RJ45 port keeps a path to the SFP+ cages and to the CPU port, so the uplink and the management interface keep working. |
| OFF | No isolation from the panel switch: all ports may talk to each other, and VLAN configuration applies as usual. |

A configuration line `vlan switch on|off` enables or disables the *function*:

- `vlan switch on` (default on boards that have the pin): the panel position
  drives isolation, as above.
- `vlan switch off`: the panel switch is ignored; isolation is left to the web
  configuration, as if the switch did not exist.

The two dimensions are separate: `vlan_switch` reports the physical position,
`vlan_switch_en` reports whether the function is enabled.

## Proposed firmware description

Two optional fields in `struct machine` (zero when absent, so existing boards
are unaffected):

```c
uint8_t vlan_switch_pin;   /* GPIO of the panel switch, 0 = none (GPIO0 is an LED) */
uint8_t sys_led_blink_idle;/* 1 = SYS LED slow blink while idle (PB-2131 manual) */
```

The board definition for `STEAMEMO_IG204_V1` / `pcb/PB_2131` sets
`vlan_switch_pin = GPIO36_PWM_OUT` and `sys_led_blink_idle = 1`. GPIO36 must be
routed to the input register: `PIN_MUX_1` bit 30 has to be set (measured; with
the bit clear the pin reads 0 in both positions). `gpio_input_setup(GPIO36)`
does that.

The main loop polls the pin with the existing SFP timer and only rewrites the
isolation registers when the position changes: for each non-SFP port,
`port_isolate(port, mask)` with `mask = all_ports | CPU_PORT` minus the other
non-SFP ports.

## Web UI

- `/information.json` gains `vlan_switch` (`"on"`/`"off"`, physical position) and
  `vlan_switch_en` (`"on"`/`"off"`, function enabled). Boards without the pin
  emit neither key.
- The dashboard gets a card with one switch labelled "toggle switch"; the hint
  reads either "the toggle switch is active. Toggle ON: the four LAN ports are
  isolated; Toggle OFF: LAN port isolation is off" or "the toggle switch is off;
  LAN port settings are made in the management UI".
- While the function is enabled, the VLAN, port isolation, port mirroring and
  LAG pages show a notice, are greyed out and their controls are read-only,
  because the panel switch owns those settings. Turning the function off from
  the dashboard restores them.

Why LAG too: with the RJ45 ports isolated from each other, a LAG whose members
are RJ45 ports cannot forward between its members (the isolation mask blocks
that egress), so the page would otherwise show a group that cannot work.

## Reference implementation

A working version (with the measured board data and the dashboard card) is in
the fork used for this report:

- `machine/STEAMEMO_IG204_V1.c` and `_init.c`
- `rtl837x_pins.c` (`vlan_switch_poll()`, `vlan_switch_json()`)
- `rtlplayground.c` (poll call next to the SFP timer)
- `cmd_parser.c` (`vlan switch on|off`)
- `httpd/page_impl.c` (JSON keys), `html/app.js` and `html/index.html` (card and
  the read-only notice)

Branch: `jzhou404gmail:sync/b1-api-reshape`
(https://github.com/jzhou404gmail/RTLPlayground/tree/sync/b1-api-reshape)

## Questions for the maintainers

1. Is a switch-specific field (`vlan_switch_pin`) acceptable, or would you
   rather describe it as a generic "isolation profile" that a board selects?
2. Should `vlan switch on` be the default for boards with the pin, and should
   the setting live in the startup configuration like other commands?
3. Is locking the VLAN/isolation/mirroring/LAG pages the right UI behaviour, or
   should only the affected controls be disabled?
4. Is `sys_led_blink_idle` worth a field of its own, or should the LED idle
   state be part of an existing LED configuration?

Hardware verified on an IG204 4+2 (PB-2131) unit: switch positions, isolation
behaviour, SFP/CPU reachability and the SYS LED idle blink were measured on
2026-10-29.
