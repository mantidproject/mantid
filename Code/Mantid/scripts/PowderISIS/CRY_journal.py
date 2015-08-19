from mantid.simpleapi import *
from getcycle import *
import time


def getJournal(instrument='HRPD', ini=0, end=1):
    # for i in range(51486,51538):
    inst = instrument[0:3]
    for i in range(ini, end + 1):
        cycle = rawpath(str(i), inst=inst)
        fname = cycle + '/' + inst + str(i) + '.raw'
        Load(Filename=fname, OutputWorkspace='Dummy')
        wksp = mtd['Dummy']
        [date_ini, time_ini] = wksp.getRun().get('run_start').value.split('T')
        [date_fin, time_fin] = wksp.getRun().get('run_end').value.split('T')
        if (i == ini):
            print '---- %s ---------------------------------------------------------------------------' % (date_ini)
        if (date_fin <> date_ini):
            print '---- %s ---------------------------------------------------------------------------' % (date_fin)
        print '%5d| %s| %6.2f|%s' % (i, time_ini, wksp.getRun().getProtonCharge(), wksp.getTitle())


# make sure we have a strptime function!
def getRunTime(instrument='HRPD', runno=0):
    inst = instrument[0:3]
    cycle = rawpath(str(runno), inst=inst)
    fname = cycle + '/' + inst + str(runno) + '.raw'
    Load(Filename=fname, OutputWorkspace='Dummy')
    wksp = mtd['Dummy']
    # [date_ini,time_ini]=
    ini = wksp.getRun().get('run_start').value
    # [date_fin,time_fin]=
    fin = wksp.getRun().get('run_end').value

    try:
        strptime = time.strptime
    except AttributeError:
        from strptime import strptime

    # print strptime("31 Nov 00", "%d %b %y")
    # print strptime("1 Jan 70 1:30pm", "%d %b %y %I:%M%p")
    return (time.mktime(strptime(fin, "%Y-%m-%dT%H:%M:%S")) - time.mktime(strptime(ini, "%Y-%m-%dT%H:%M:%S"))) / 3600


def getRunTimeFILE(fname):
    Load(Filename=fname, OutputWorkspace='Dummy')
    wksp = mtd['Dummy']
    # [date_ini,time_ini]=
    ini = wksp.getRun().get('run_start').value
    # [date_fin,time_fin]=
    fin = wksp.getRun().get('run_end').value

    try:
        strptime = time.strptime
    except AttributeError:
        from strptime import strptime

    # print strptime("31 Nov 00", "%d %b %y")
    # print strptime("1 Jan 70 1:30pm", "%d %b %y %I:%M%p")
    return (time.mktime(strptime(fin, "%Y-%m-%dT%H:%M:%S")) - time.mktime(strptime(ini, "%Y-%m-%dT%H:%M:%S"))) / 3600


if __name__ == '__main__':
    # getJournal(instrument='HRPD', cycle='cycle_11_4', ini=51486, end=51536)
    getRunTime(instrument='HRPD', cycle='cycle_12_3', runno=54797)

