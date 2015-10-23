import stresstesting
from mantid.simpleapi import *
from mantid.api import *



class SplineSmoothingTest(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return ["Stheta.nxs", "ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs"]

    def runTest(self):
        self._success = False

        wsenginx = Load("ENGINX_precalculated_vanadium_run000236516_bank_curves")
        SplineSmoothing(InputWorkspace=wsenginx, OutputWorkspace="enginxOutSpline", MaxNumberOfBreaks=20)

        # MaxNumberOfBreaks=0, maximum number of breaking points possible
        wsStheta = Load("Stheta")
        SplineSmoothing(InputWorkspace=wsStheta, OutputWorkspace="SthetaOutSpline")

        self._success = True

    def validate(self):
        return self._success
