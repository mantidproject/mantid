
from __future__ import (absolute_import, division, print_function)
# exactly as-is (will need to write to Mantid workspaces instead)


def OUTPUT(POINTS_npts, RUNDATA_res, RUNDATA_irunno, MAXPAGE_n, MAXPAGE_f):
    fperchan = 1. / (RUNDATA_res * float(POINTS_npts) * 2.)
    fname = "{:08d}.max".format(RUNDATA_irunno)  # new longer run numbers!
    fil = open(fname, "w")
    fil.write(
              "{:8d} {:8.6} {:5d} {:4d} {:4d}".format(
                RUNDATA_irunno,
                RUNDATA_res,
                POINTS_npts,
                1,
                MAXPAGE_n))
    for i in range(MAXPAGE_n):
        fil.write(
            "{:13.4e} {:10.2f}".format(MAXPAGE_f[i],
                                       float(i) * fperchan / 135.5E-4))
    fil.close()
