import stresstesting
from mantid.simpleapi import *
from mantid.api import *
import unittest
DIFF_PLACES = 7

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

        # Custom code to create and run this single test suite
        # and then mark as success or failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadTests, "test"))
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success


# ======================================================================
# work horse
class LoadTests(unittest.TestCase):
    wsname = "__LoadTest"
    cleanup_names = []
    # ws_smooth = mtd['enginxOutSpline']

    def tearDown(self):
        self.cleanup_names.append(self.wsname)
        for name in self.cleanup_names:
            try:
                AnalysisDataService.remove(name)
            except KeyError:
                pass
        self.cleanup_names = []

    # ============================ Success ==============================
    def runTest(self):
        wsenginx = Load("ENGINX_precalculated_vanadium_run000236516_bank_curves")
        ws_smooth = SplineSmoothing(InputWorkspace=wsenginx, MaxNumberOfBreaks=50)

        # MaxNumberOfBreaks=0, maximum number of breaking points possible
        wsStheta = Load("Stheta")
        SplineSmoothing(InputWorkspace=wsStheta, OutputWorkspace="SthetaOutSpline")

    def test_Enginx_curve_spline_smoothed(self):
        ws_smooth = mtd['enginxOutSpline']

        # SplineSmooth all three spectrum
        self.assertEquals(3, ws_smooth.getNumberHistograms())
        self.assertEquals(14168, ws_smooth.blocksize())

        self.assertAlmostEqual(0.24360103, ws_smooth.readX(2)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.86546920, ws_smooth.readX(2)[1738], places=DIFF_PLACES)

    def test_STheta_curve_spline_smoothed(self):
        ws_smooth = mtd['SthetaOutSpline']

        # SplineSmooth all three spectrum
        self.assertEquals(1, ws_smooth.getNumberHistograms())
        self.assertEquals(463, ws_smooth.blocksize())

        self.assertAlmostEqual(2.3405, ws_smooth.readX(0)[0], places=4)
        self.assertAlmostEqual(56.9972, ws_smooth.readX(0)[231], places=4)
