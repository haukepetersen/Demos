# name of your application
APPLICATION = node_iotlab-m3

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../RIOT

WITH_SHELL ?= 0

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_sixlowpan_iphc_nhc
USEMODULE += gnrc_udp
USEMODULE += gnrc_conn_udp
ifeq (1, $(WITH_SHELL))
USEMODULE += gnrc_icmpv6_echo
USEMODULE += shell
USEMODULE += shell_commands
endif
# Add the sensors
USEMODULE += isl29020
USEMODULE += lps331ap

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP -DAT86RF2XX_DEFAULT_CHANNEL=26
ifeq (1, $(WITH_SHELL))
CFLAGS += -DWITH_SHELL
endif

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
