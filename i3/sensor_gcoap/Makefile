# name of your application
APPLICATION = i3_sensor_gcoap

# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../RIOT

# We going to need some parts of RIOT
USEMODULE += luid
USEMODULE += fmt
USEMODULE += gcoap
USEMODULE += saul_default
# Include GNRC
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
# Also we want some statistics
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl

# add some useful tools
USEMODULE += shell
USEMODULE += shell_commands

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# we use a different channel for the demo
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=17

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
