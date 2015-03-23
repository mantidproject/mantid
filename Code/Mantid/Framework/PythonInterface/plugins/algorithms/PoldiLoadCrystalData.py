# pylint: disable=no-init,invalid-name,bare-except
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *


class PoldiLoadCrystalData(PythonAlgorithm):
    def category(self):
        return "SINQ\\POLDI"

    def name(self):
        return "PoldiLoadCrystalData"

    def summary(self):
        return ("The algorithm reads a POLDI crystal structure file and creates a WorkspaceGroup that contains tables"
                "with the expected reflections.")

    def PyInit(self):
        pass

    def PyExec(self):
        pass


AlgorithmFactory.subscribe(PoldiLoadCrystalData)