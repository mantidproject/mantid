# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-name-in-module,too-many-public-methods
import numpy as np
from mantid.simpleapi import LoadEventNexus


def LoadNexusUB(fname):
    raw_workspace = LoadEventNexus(Filename=fname, MetaDataOnly=True, StoreInADS=False)
    run = raw_workspace.getRun()
    run_keys = run.keys()
    UBMatrix_key = [test_key for test_key in run_keys if "UBMatrix" in test_key][0]
    UB = np.array(eval(run[UBMatrix_key].value[0].strip(")").split("(")[1]))
    return UB
