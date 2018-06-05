#pylint: disable=invalid-name,no-name-in-module,too-many-public-methods
from __future__ import (absolute_import, division, print_function)
import numpy as np
from mantid.simpleapi import *


def LoadNexusUB(fname):
    raw=LoadEventNexus(Filename=fname)
    run = raw.getRun()
    run_keys = run.keys()
    for test_key in run_keys:
        if test_key[-8:] == 'UBMatrix':
            UBMatrix_key = test_key
    UB=np.array( eval(raw.run()[UBMatrix_key].value[0].strip(')').split('(')[1]))
    return UB
