# I3 Demonstrator - MQTT-SN-based

## Steps to get it to run:

- Border router: start `gnrc_border_router` (with `DEFAULT_CHANNEL=17`):
```
DEFAULT_CHANNEL=17 make all flash term
```
- Add out custom address to the tap device
```
sudo ip a a affe::1 dev tap0
```
- Flash and run

- run listener (sub to topic using mosquitto)
```
mosquitto_sub -h ::1 -p 1886 -t "#" -v
```


## Note: watch for SERIAL:

build with `SERIAL=xxx` when using sam and phynode at the same time

get serials with `make list-ttys`
