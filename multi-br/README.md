# Raspberry Pi 2 as BLE and IEEE802.15.4 Border Router

Through 6LoWPAN, BLE and IEEE802.15.4 link layers can be used to connect tiny
constrained devices to natively speak IPv6, and therefore connect them to the
great evil Internet. One building block needed for this though, is a 6LoWPAN
border router.

This guide documents my experiences setting up a Raspberry Pi 2 as dual-stack
border router, connecting RIOT based IEEE802.15.4 nodes as well as BLE nodes
based on Nordic's IoT_SDK to the Internet.

In this setup,

## Prerequisites

#### Hardware used
- Raspberry Pi 2
- some cheap, no-name, Bluetooth 4.0 capable USB dongle
```
lsusb says: Bus 001 Device 004: ID 0a5c:21ec Broadcom Corp.
```
- OpenLabs AT86RF233 IEEE802.15.4 adapter (http://openlabs.co/OSHW/Raspberry-Pi-802.15.4-radio)
- Nordic `nRF52dk` (pca100040)
- Atmel `samr21-xpro`


#### Software used
On the Pi:
- raspian (jessie-minimal)
- home-brew linux kernel (see below for more on this):
```
root@riot-br1:/home/pi# uname -a
Linux riot-br1 4.1.19-v7+ #858 SMP Tue Mar 15 15:56:00 GMT 2016 armv7l GNU/Linux
```
- radvd
- wpan-tools

On the nRF52dk:
- nrf52_iot_sdk_v0.9.x
- using the `coap/ipv6/server example` from this

On the samr21-xpro:
- RIOT (just take the current master)
- using the `examples/gnrc_networking` application

On your host computer:
- some Linux, I am using Mint 17 (OSX or Windows might work, but don't ask me how...)
- toolchain to build the Linux kernel for the Pi:
```
$ arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-gcc (Linaro GCC 4.8-2015.06) 4.8.5
```
- toolchain for building RIOT and the Nordic SDK:
```
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (GNU Tools for ARM Embedded Processors) 4.8.4 20140725 (release) [ARM/embedded-4_8-branch revision 213147]
```
- some other build tools, e.g. `git`, etc.

#### Network configuration
For playing around, we use the following 'global' unicast address configuration:
- Pi, dev `bt0`: `2001:affe:0:1::1/64`
- prefix propagated to the BLE subnet: `2001:affe:0:1::/64`
- Pi, dev `lowpan0`: `2001:affe:0:2::1/64`
- prefix propagated to the 802.15.4 subnet: `2001:affe:0:2::/64`



## Setting up the `samr21-xpro`
1. check out RIOT
2. go to RIOT/examples/gnrc_networking
3. compile the application for the board:
```
$ BOARD=samr21-xpro make
```
4. flash the application to the board:
```
$ BOARD=samr21-xpro make flash
```
5. connect to the RIOT shell and have fun
```
$ BOARD=samr21-xpro make term
```
6. try `help` to see the shell commands, or `ifconfig` to see the network configuration on the node


## Setting up the `nRF52dk`
I chose to do my trial runs with the
[coap/server example application](http://developer.nordicsemi.com/nRF5_IoT_SDK/doc/0.9.0/html/a00052.html),
here is what I have done to build this under Linux Mint 17:

1. Download the [Nordic IoT SDK, V0.9.x](https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/)
2. go to the example application
```
$ cd PATH_TO_SDK/examples/iot/coap/ipv6/server/boards/pca10040/armgcc
```
3. before building it, I had to do some adjustments to the shipped Makefile:
  - toolchain: path and PREFIX did not work for me, as my toolchain is located in some non standard path,
    but its in the `PATH` environment variable (useful when switching a lot between different versions)
  - flash target: added the device `-f nrf52` to the reset command
  - flash_softdevice target: added this for convenience:
```
flash_softdevice:
    @echo Flashing: s132_nrf52_2.0.0_softdevice.hex
    nrfjprog --program ../../../../../../../../components/softdevice/s1xx_iot/s1xx-iot-prototype3_nrf52_softdevice.hex -f nrf52 --sectorerase
    nrfjprog --reset -f nrf52
```
  - also I copied in some other things that I found in other Makefiles, they are probably not needed, but they also
    seemed not to hurt... [Here](https://github.com/haukepetersen/Demos/blob/master/multi-br/Makefile) is a copy of the Makefile that I used.

4. flash the softdevice
```
$ make flash_softdevice
```
5. build the exmaple application
```
$ make
```
6. flash the example application
```
$ make flash
```
7. This should be it, LED1 on the board should now be blinking...


## Setting up the Pi

### Installation
Setting up the Pi might be the most time consuming task here, but it actually is pretty straight forward.
I basically followed
[these 3 tutorials](https://github.com/RIOT-Makers/wpan-raspbian/wiki),
with some alterations:

- I used kernel version `4.1.x`! `4.4.x` did not work for me for unknown reasons.
- make sure the `bluetooth_6lowpan` module is included
- set the default configuration for the 802.15.4 adapter (`/etc/default/lowpan`) to:
```
CHN="26"
PAN="0x23"
MAC="18:C9:FF:EE:1A:CA:AB:CD" # set to "" if not applicable
```
Besides these things, just follow the instructions step-by-step, and everything should work :-)


### Configure `radvd`
For the Pi to send out router advertisements, we use the patched version of `radvd` as described in
the tutorial linked above.
[This is the radvd configuration that worked for me](https://github.com/haukepetersen/Demos/blob/master/multi-br/radvd.conf)


### Address configuration
Add global unicast addresses to `bt0` and `lowpan0` interfaces:
```
pi@riot-br1:~ $ sudo ip a a 2001:affe:0:2::1/64 dev lowpan0
pi@riot-br1:~ $ sudo ip a a 2001:affe:0:1::1/64 dev bt0
```

### Start `radvd`
```
pi@riot-br1:~ $ sudo radvd -d 5 -m stderr -n
```



## Results

#### Collecting some addresses

For some pinging action, we calculate the public address of the BLE node, using
[following this guide](http://developer.nordicsemi.com/nRF5_IoT_SDK/doc/0.9.0/html/a00088.html),
which I quickly poured into [this tool](https://github.com/haukepetersen/Demos/blob/master/multi-br/bt_to_ipv6.py):
```
$ ./bt_to_ipv6.py 00:8C:56:24:03:EF
Bluetooth device address: 00:8c:56:24:03:ef
IPv6 link local address:  fe80::08c:56ff:fe24:03ef
IPv6 public address:      fe80::28c:56ff:fe24:03ef
```
The RIOT node has the following addresses (run `ifconfig`) on the node:
```
2016-04-19 19:16:36,049 - INFO # > ifconfig
2016-04-19 19:16:36,051 - INFO # Iface  7   HWaddr: 2c:2a  Channel: 26  Page: 0  NID: 0x23
2016-04-19 19:16:36,052 - INFO #            Long HWaddr: 36:32:48:33:46:da:ac:2a
2016-04-19 19:16:36,054 - INFO #            TX-Power: 0dBm  State: IDLE  max. Retrans.: 3  CSMA Retries: 4
2016-04-19 19:16:36,056 - INFO #            AUTOACK  CSMA  MTU:1280  HL:255  6LO  RTR  RTR_ADV  IPHC
2016-04-19 19:16:36,057 - INFO #            Source address length: 8
2016-04-19 19:16:36,057 - INFO #            Link type: wireless
2016-04-19 19:16:36,058 - INFO #            inet6 addr: ff02::1/128  scope: local [multicast]
2016-04-19 19:16:36,060 - INFO #            inet6 addr: fe80::3432:4833:46da:ac2a/64  scope: local
2016-04-19 19:16:36,061 - INFO #            inet6 addr: ff02::1:ffda:ac2a/128  scope: local [multicast]
2016-04-19 19:16:36,063 - INFO #            inet6 addr: 2001:affe:0:2:3432:4833:46da:ac2a/64  scope: global
2016-04-19 19:16:36,064 - INFO #            inet6 addr: ff02::2/128  scope: local [multicast]
```

Now we are ready to ping a little bit between our devices...

Ping from the Pi to the RIOT node, link local and global addressing:
```
pi@riot-br1:~ $ ping6 -c3 fe80::3432:4833:46da:ac2a%lowpan0
PING fe80::3432:4833:46da:ac2a%lowpan0(fe80::3432:4833:46da:ac2a) 56 data bytes
64 bytes from fe80::3432:4833:46da:ac2a: icmp_seq=1 ttl=255 time=9.70 ms
64 bytes from fe80::3432:4833:46da:ac2a: icmp_seq=2 ttl=255 time=10.4 ms
64 bytes from fe80::3432:4833:46da:ac2a: icmp_seq=3 ttl=255 time=9.47 ms

--- fe80::3432:4833:46da:ac2a%lowpan0 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 9.473/9.885/10.474/0.427 ms
```
and
```
pi@riot-br1:~ $ ping6 -c3 2001:affe:0:2:3432:4833:46da:ac2a
PING 2001:affe:0:2:3432:4833:46da:ac2a(2001:affe:0:2:3432:4833:46da:ac2a) 56 data bytes
64 bytes from 2001:affe:0:2:3432:4833:46da:ac2a: icmp_seq=1 ttl=255 time=13.7 ms
64 bytes from 2001:affe:0:2:3432:4833:46da:ac2a: icmp_seq=2 ttl=255 time=13.5 ms
64 bytes from 2001:affe:0:2:3432:4833:46da:ac2a: icmp_seq=3 ttl=255 time=13.1 ms

--- 2001:affe:0:2:3432:4833:46da:ac2a ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2003ms
rtt min/avg/max/mdev = 13.159/13.458/13.707/0.263 ms
```

And from the Pi to the BLE node:
```
pi@riot-br1:~ $ ping6 -c3 fe80::28c:56ff:fe24:03ef%bt0
PING fe80::28c:56ff:fe24:03ef%bt0(fe80::28c:56ff:fe24:3ef) 56 data bytes
64 bytes from fe80::28c:56ff:fe24:3ef: icmp_seq=1 ttl=64 time=156 ms
64 bytes from fe80::28c:56ff:fe24:3ef: icmp_seq=2 ttl=64 time=169 ms
64 bytes from fe80::28c:56ff:fe24:3ef: icmp_seq=3 ttl=64 time=181 ms

--- fe80::28c:56ff:fe24:03ef%bt0 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2000ms
rtt min/avg/max/mdev = 156.798/169.384/181.416/10.069 ms
```
and
```
pi@riot-br1:~ $ ping6 -c3 2001:affe:0:1:28c:56ff:fe24:03ef
PING 2001:affe:0:1:28c:56ff:fe24:03ef(2001:affe:0:1:28c:56ff:fe24:3ef) 56 data bytes
64 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: icmp_seq=1 ttl=64 time=253 ms
64 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: icmp_seq=2 ttl=64 time=265 ms
64 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: icmp_seq=3 ttl=64 time=207 ms

--- 2001:affe:0:1:28c:56ff:fe24:03ef ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 207.957/242.031/265.032/24.581 ms
```


**And now the most interesting: Ping from RIOT node to the BLE node:**
```
2016-04-19 19:16:59,470 - INFO # > ping6 2001:affe:0:1:28c:56ff:fe24:03ef
2016-04-19 19:16:59,607 - INFO # 12 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: id=83 seq=1 hop limit=63 time = 134.831 ms
2016-04-19 19:17:00,754 - INFO # 12 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: id=83 seq=2 hop limit=63 time = 91.527 ms
2016-04-19 19:17:01,902 - INFO # 12 bytes from 2001:affe:0:1:28c:56ff:fe24:3ef: id=83 seq=3 hop limit=63 time = 125.538 ms
2016-04-19 19:17:01,903 - INFO # --- 2001:affe:0:1:28c:56ff:fe24:03ef ping statistics ---
2016-04-19 19:17:01,905 - INFO # 3 packets transmitted, 3 received, 0% packet loss, time 2.06431765 s
2016-04-19 19:17:01,906 - INFO # rtt min/avg/max = 91.527/117.298/134.831 ms
```

So awesome: we have a full IPv6 connection between two little micro controllers
over two different link layer technologies -> IoT, here we come!


## Next steps

Next it would be fun, to play a little more with UDP and CoAP on these devices,
another day...
