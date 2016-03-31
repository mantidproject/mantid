#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *

import math

class EnggFitDIFCFromPeaks(PythonAlgorithm):

    def category(self):
        return "Diffraction\\Engineering"

    def name(self):
        return "EnggFitPeaks"

    def summary(self):
        return ("The algorithm fits an expected diffraction pattern to a workpace spectrum by "
                "performing single peak fits.")

    def PyInit(self):
        pass

    def PyExec(self):
        pass

AlgorithmFactory.subscribe(EnggFitPeaks)
