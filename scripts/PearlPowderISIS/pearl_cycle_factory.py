# pylint: disable=anomalous-backslash-in-string, too-many-branches, superfluous-parens

from mantid.simpleapi import *
import pearl_routines


def get_cycle_dir(number, currentdatadir):
    if type(number) is int:
        runno = number
    else:
        num = number.split("_")
        runno = int(num[0])

    pearl_raw_dir = "X:\data\cycle_"
    print "The run number is get cycle is", runno
    if (runno < 71009):
        cycle = "10_2"
        print "cycle is set to", cycle
        instver = "old"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 71084):
        cycle = "11_1"
        print "cycle is set to", cycle
        instver = "new"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 71991):
        cycle = "11_2"
        print "cycle is set to", cycle
        instver = "new"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 73064):
        cycle = "11_3"
        print "cycle is set to", cycle
        instver = "new"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 73984):
        cycle = "11_4"
        print "cycle is set to", cycle
        instver = "new"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 74757):
        cycle = "11_5"
        print "cycle is set to", cycle
        instver = "new"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 75552):
        cycle = "12_1"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 76662):
        cycle = "12_2"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 77677):
        cycle = "12_3"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 78610):
        cycle = "12_4"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 80073):
        cycle = "12_5"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 80812):
        cycle = "13_1"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 81280):
        cycle = "13_2"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 82317):
        cycle = "13_3"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 82764):
        cycle = "13_4"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 84841):
        cycle = "13_5"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 86631):
        cycle = "14_1"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 88087):
        cycle = "14_2"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 89389):
        cycle = "14_3"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 90481):
        cycle = "15_1"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 91530):
        cycle = "15_2"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(datadir)
    elif (runno < 92432):
        cycle = "15_3"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(currentdatadir)
    else:
        cycle = "15_4"
        print "cycle is set to", cycle
        instver = "new2"
        datadir = pearl_raw_dir + cycle + "\\"
        pearl_routines.PEARL_setdatadir(currentdatadir)

    return cycle, instver, datadir
