# Time

The switch has no battery backed clock. It counts seconds from power on, and
it can take the date and time from an NTP server once the network is up. The
time is lost at every restart and set again at the first synchronisation.

```
ntp server <name|ip>     # default ntp.tencent.com
ntp interval <1-1440>    # minutes between two synchronisations, default 60
ntp timezone <+hh:mm>    # offset from UTC, -12:00 to +14:00, default +08:00
ntp dst off|eu|us        # daylight saving rule
ntp on
ntp off
ntp                      # settings and time
time                     # local date and time
```

The client speaks SNTP version 4 and synchronises at once when turned on,
then after every interval. A server given by name is resolved through the DNS
resolver before every synchronisation. An answer from an unsynchronised
server is ignored, and a failed attempt is repeated after 30 seconds.

A name can only be resolved once the switch has a DNS server: it has none of
its own, a board configured with a static address and without a `dns server`
line asks nobody, and DHCP hands one out only when the address comes from
DHCP. `ntp` and `time` say which of the two is missing, and so does the Time
card of the System page:

```
Time not synchronised, no DNS server for ntp.example.com
Time not synchronised, cannot resolve ntp.example.com
Time not synchronised, no usable answer from 203.0.113.9
```

An IPv4 address needs no lookup at all and works without a DNS server.

The local time is UTC plus the time zone offset, plus one hour while daylight
saving time applies:

* `eu`: from 01:00 UTC on the last Sunday of March to 01:00 UTC on the last
  Sunday of October;
* `us`: from 02:00 local time on the second Sunday of March to 02:00 local
  time on the first Sunday of November.

The same settings, with the current local time and the last synchronisation,
are in the Time card of the System page. Settings made there or on the
console are kept only once the configuration is saved.
