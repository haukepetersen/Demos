#!/usr/bin/env python

# Copyright (C) 2015  Hauke Petersen <dev@haukepetersen.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import re

def main( ):
    if (len(sys.argv) != 2):
        sys.exit("Usage: %s <Bluetooth device address>" % sys.argv[0])

    arg = sys.argv[1].lower()
    bda = re.match("^([0-9a-z]{1,2}:){5}[0-9a-z]{1,2}$", arg)
    if not bda:
        sys.exit("Error: invalid Bluetooth device address, must have format xx:xx:xx:xx:xx:xx")

    ap = arg.split(':')

    msb = int(ap[0], 16)
    msb_laa = (msb | 2) ^ 2
    msb_pub = (msb | 2)

    lla = ("fe80::%x%s:%sff:fe%s:%s%s" % (msb_laa, ap[1], ap[2], ap[3], ap[4], ap[5]))
    pub = ("fe80::%x%s:%sff:fe%s:%s%s" % (msb_pub, ap[1], ap[2], ap[3], ap[4], ap[5]))

    print("Bluetooth device address: %s" % arg)
    print("IPv6 link local address:  %s" % lla)
    print("IPv6 public address:      %s" % pub)

if __name__ == "__main__":
    main()
