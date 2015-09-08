# name of your application
APPLICATION = door

# If no BOARD is found in the environment, use this default:
BOARD ?= samr21-xpro

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../../RIOT

# lets use the shell for debugging
USEMODULE += shell
USEMODULE += shell_commands
# include the srf02 sensor driver
USEMODULE += srf02
# include the xtimer
USEMODULE += xtimer
# and include the gnrc stack
USEMODULE += gnrc_netif_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_udp
# USEMODULE += gnrc_icmpv6_echo

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Include application specific headers
INCLUDES += -I$(CURDIR)/include

include $(RIOTBASE)/Makefile.include
