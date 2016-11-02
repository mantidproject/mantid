# pylint: disable=anomalous-backslash-in-string, too-many-branches, superfluous-parens

from __future__ import (absolute_import, division, print_function)


def get_cycle_dir(number):
    if isinstance(number, int):
        runno = number
    else:
        num = number.split("_")
        runno = int(num[0])

    if (runno < 71009):
        cycle = "10_2"
        instver = "old"

    elif (runno < 71084):
        cycle = "11_1"
        instver = "new"

    elif (runno < 71991):
        cycle = "11_2"
        instver = "new"

    elif (runno < 73064):
        cycle = "11_3"
        instver = "new"

    elif (runno < 73984):
        cycle = "11_4"
        instver = "new"

    elif (runno < 74757):
        cycle = "11_5"
        instver = "new"

    elif (runno < 75552):
        cycle = "12_1"
        instver = "new2"

    elif (runno < 76662):
        cycle = "12_2"
        instver = "new2"

    elif (runno < 77677):
        cycle = "12_3"
        instver = "new2"

    elif (runno < 78610):
        cycle = "12_4"
        instver = "new2"

    elif (runno < 80073):
        cycle = "12_5"
        instver = "new2"

    elif (runno < 80812):
        cycle = "13_1"
        instver = "new2"

    elif (runno < 81280):
        cycle = "13_2"
        instver = "new2"

    elif (runno < 82317):
        cycle = "13_3"
        instver = "new2"

    elif (runno < 82764):
        cycle = "13_4"
        instver = "new2"

    elif (runno < 84841):
        cycle = "13_5"
        instver = "new2"

    elif (runno < 86631):
        cycle = "14_1"
        instver = "new2"

    elif (runno < 88087):
        cycle = "14_2"
        instver = "new2"

    elif (runno < 89389):
        cycle = "14_3"
        instver = "new2"

    elif (runno < 90481):
        cycle = "15_1"
        instver = "new2"

    elif (runno < 91530):
        cycle = "15_2"
        instver = "new2"

    elif (runno < 92432):
        cycle = "15_3"
        instver = "new2"

    else:
        cycle = "15_4"
        instver = "new2"

    return cycle, instver
