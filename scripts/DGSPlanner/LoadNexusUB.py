#pylint: disable=invalid-name,no-name-in-module,too-many-public-methods
from __future__ import (absolute_import, division, print_function)
import numpy as np
from mantid.simpleapi import *


def LoadNexusUB(fname):
    __raw=LoadEventNexus(Filename=fname, MetaDataOnly=True)
    run = __raw.getRun()
    run_keys = run.keys()
    UBMatrix_key = [test_key for test_key in run_keys if 'UBMatrix' in test_key][0]
    UB=np.array( eval(raw.run()[UBMatrix_key].value[0].strip(')').split('(')[1]))
    DeleteWorkspace(__raw)
    return UB
