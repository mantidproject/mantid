# pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from mantid.geometry import CrystalStructure


class PredictPeaksTest(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='WISH',
                                       BinParams='0,1,2',
                                       UnitX='TOF')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(ws,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub.nxs')

        wsMatch = CheckWorkspacesMatch(peaks, reference)

        self.assertTrue(wsMatch)


class PredictPeaksCalculateStructureFactorsTest(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='WISH',
                                       BinParams='0,1,2',
                                       UnitX='TOF')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')

        # Setting some random crystal structure. Correctness of structure factor calculations is ensured in the
        # test suite of StructureFactorCalculator and does not need to be tested here.
        ws.sample().setCrystalStructure(
            CrystalStructure('5.5 6.5 8.1', 'P m m m', 'Fe 0.121 0.234 0.899 1.0 0.01'))

        peaks = PredictPeaks(ws,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10,
                             CalculateStructureFactors=True)

        self.assertEquals(peaks.getNumberPeaks(), 540)

        for i in range(540):
            peak = peaks.getPeak(i)
            self.assertLessThan(0.0, peak.getIntensity())

        peaks_no_sf = PredictPeaks(ws,
                                   WavelengthMin=0.5, WavelengthMax=6,
                                   MinDSpacing=0.5, MaxDSpacing=10,
                                   CalculateStructureFactors=False)

        for i in range(540):
            peak = peaks_no_sf.getPeak(i)
            self.assertEquals(0.0, peak.getIntensity())
