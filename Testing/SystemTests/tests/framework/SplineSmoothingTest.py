# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

import systemtesting
from mantid.api import mtd, AnalysisDataService
from mantid.simpleapi import Load, SplineSmoothing
import unittest

DIFF_PLACES = 7


class SplineSmoothingTest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        return set(["Stheta.nxs", "ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs"])

    def runTest(self):
        self._success = False

        wsenginx = Load("ENGINX_precalculated_vanadium_run000236516_bank_curves")
        SplineSmoothing(InputWorkspace=wsenginx, OutputWorkspace="enginxOutSpline", MaxNumberOfBreaks=20)

        # MaxNumberOfBreaks=0, maximum number of breaking points possible
        wsstheta = Load("Stheta")
        SplineSmoothing(InputWorkspace=wsstheta, OutputWorkspace="SthetaOutSpline")

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
        SplineSmoothing(InputWorkspace=wsenginx, MaxNumberOfBreaks=50)

        # MaxNumberOfBreaks=0, maximum number of breaking points possible
        wsstheta = Load("Stheta")
        SplineSmoothing(InputWorkspace=wsstheta, OutputWorkspace="SthetaOutSpline")

    def test_enginx_curve_spline_smoothed(self):
        ws_smooth = mtd["enginxOutSpline"]

        # SplineSmooth all three spectrum
        self.assertEqual(3, ws_smooth.getNumberHistograms())
        self.assertEqual(14168, ws_smooth.blocksize())

        self.assertAlmostEqual(0.24360103, ws_smooth.readX(2)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.86546920, ws_smooth.readX(2)[1738], places=DIFF_PLACES)

        self.assertAlmostEqual(57.1213842, ws_smooth.readY(2)[0], places=DIFF_PLACES)

    def test_stheta_curve_spline_smoothed(self):
        ws_smooth = mtd["SthetaOutSpline"]

        # SplineSmooth all three spectrum
        self.assertEqual(1, ws_smooth.getNumberHistograms())
        self.assertEqual(463, ws_smooth.blocksize())

        self.assertAlmostEqual(2.3405, ws_smooth.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(56.9972, ws_smooth.readX(0)[231], places=DIFF_PLACES)

    def test_enginx_curve_further_checks(self):
        ws_smooth_enginx = mtd["enginxOutSpline"]
        ws_enginx = mtd["wsenginx"]

        ws_smooth_stetha = mtd["SthetaOutSpline"]
        ws_stetha = mtd["wsStheta"]

        for i in range(0, 462):
            self.assertTrue(ws_enginx.readX(0)[i], ws_smooth_enginx.readX(0)[i])
            self.assertTrue(ws_stetha.readX(0)[i], ws_smooth_stetha.readX(0)[i])
            self.assertTrue(ws_stetha.readY(0)[i], ws_smooth_stetha.readY(0)[i])
