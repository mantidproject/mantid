#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class PredictPeaksTest(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='WISH',
                                  BinParams='0,0.1,0.2',
                                  UnitX='dSpacing')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(ws,
                     WavelengthMin=0.5, WavelengthMax=6,
                     MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub.nxs')

        wsMatch = CheckWorkspacesMatch(peaks, reference)

        self.assertTrue(wsMatch)