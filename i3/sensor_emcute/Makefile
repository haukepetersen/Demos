# name of your application
APPLICATION = i3_sensor_emcute

# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# We going to need some parts of RIOT
USEMODULE += uuid
USEMODULE += emcute
USEMODULE += saul_reg

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
