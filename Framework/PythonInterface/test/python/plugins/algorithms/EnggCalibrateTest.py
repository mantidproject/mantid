# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest
import mantid.simpleapi as sapi


class EnggCalibrateTest(unittest.TestCase):
    _data_ws = None
    _van_curves_ws = None
    _van_integ_tbl = None

    # Note not using @classmethod setUpClass / tearDownClass because that's not supported in the old
    # unittest of rhel6
    def setUp(self):
        """
        Set up dependencies for one or more of the tests below.
        """
        if not self.__class__._data_ws:
            self.__class__._data_ws = sapi.LoadNexus("ENGINX00228061.nxs", OutputWorkspace="ENGIN-X_test_ws")

        if not self.__class__._van_curves_ws:
            # Note the pre-calculated file instead of the too big vanadium run
            # self.__class__._van_ws = LoadNexus("ENGINX00236516.nxs", OutputWorkspace='ENGIN-X_test_vanadium_ws')
            self.__class__._van_curves_ws = sapi.LoadNexus(
                Filename="ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs", OutputWorkspace="ENGIN-X_vanadium_curves_test_ws"
            )
            self.__class__._van_integ_tbl = sapi.LoadNexus(
                Filename="ENGINX_precalculated_vanadium_run000236516_integration.nxs", OutputWorkspace="ENGIN-X_vanadium_integ_test_ws"
            )

    def test_issues_with_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required
        ones.
        """

        # No InputWorkspace property (required)
        self.assertRaises(TypeError, sapi.EnggCalibrate, File="foo", Bank="1")

        # Wrong (mispelled) InputWorkspace property
        self.assertRaises(TypeError, sapi.EnggCalibrate, InputWorkpace="anything_goes", Bank="2")

        # mispelled ExpectedPeaks
        tbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace="test_table")
        self.assertRaises(TypeError, sapi.EnggCalibrate, Inputworkspace=self.__class__._data_ws, DetectorPositions=tbl, Bank="2", Peaks="2")

        # mispelled DetectorPositions
        self.assertRaises(TypeError, sapi.EnggCalibrate, InputWorkspace=self.__class__._data_ws, Detectors=tbl, Bank="2", Peaks="2")

        # There's no output workspace
        self.assertRaises(TypeError, sapi.EnggCalibrate, InputWorkspace=self.__class__._data_ws, Bank="1")

    def test_fails_gracefully(self):
        """
        Checks a bad run.
        """

        # This should produce 'given peak center ... is outside of data range' warnings
        # and finally raise after a 'some peaks not found' error
        self.assertRaises(TypeError, sapi.EnggCalibrate, InputWorkspace=self.__class__._data_ws, ExpectedPeaks=[0.2, 0.4], Bank="2")

    def test_runs_ok_without_reliable_peaks(self):
        """
        Checks normal operation.
        """
        try:
            difa, difc, zero, peaks = sapi.EnggCalibrate(
                InputWorkspace=self.__class__._data_ws,
                VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                VanCurvesWorkspace=self.__class__._van_curves_ws,
                ExpectedPeaks=[1.6, 1.1, 1.8],
                Bank="2",
            )
            self.assertEqual(difa, 0)
            self.assertGreater(difc, 0)
            self.assertLess(abs(zero), 1000)
            self.assertEqual(peaks.rowCount(), 3)

        except RuntimeError:
            pass

    def test_runs_ok_with_bad_peaks_file(self):
        """
        Normal operation with a csv input file with (poor / impossible) expected peaks
        """
        # This file has: 1.6, 1.1, 1.8 (as the test above)
        filename = "EnginX_3_expected_peaks_unittest.csv"
        try:
            difa, difc, zero, peaks = sapi.EnggCalibrate(
                InputWorkspace=self.__class__._data_ws,
                VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                VanCurvesWorkspace=self.__class__._van_curves_ws,
                # nonsense, but FromFile should prevail
                ExpectedPeaks=[-4, 40, 323],
                ExpectedPeaksFromFile=filename,
                Bank="2",
            )
            self.assertEqual(difa, 0)
            self.assertGreater(difc, 0)
            self.assertLess(abs(zero), 1000)
            self.assertEqual(peaks.rowCount(), 3)

        except RuntimeError:
            pass

    def check_3peaks_values(self, difa, difc, zero):
        self.assertEqual(difa, 0)

        # There are platform specific differences in final parameter values
        # For example in earlier versions, debian: 369367.57492582797; win7: 369242.28850305633
        # osx were ~0.995% different (18920.539474 instead of 18485.223143) in the best case but can
        # be as big as ~7.6% different (17066.631460 instead of 18485.223143)
        # win7 with MSVC 2015 results were ~4.1% different  from linux expected values,
        if "darwin" == sys.platform or "win32" == sys.platform:
            return
        else:
            expected_difc = 18446.903615
            expected_zero = 416.082164
        difc_err_epsilon = 1e-2

        # assertLess would be nice, but only available in unittest >= 2.7
        self.assertTrue(
            abs((expected_difc - difc) / expected_difc) < difc_err_epsilon,
            "DIFC (%f) is too far from its expected value (%f)" % (difc, expected_difc),
        )

        # especially this zero parameter is extremely platform dependent/sensitive
        # Examples:
        # ubuntu: -724.337354; win7: -995.879786, osx: -396.628396
        # ubuntu: 416.082164, win7: 351.580794, osx: (didn't even try as difc was already nearly 10% different)

        # this is obviously a ridiculous threshold to do just a very rough test that results/functionality
        # do not change too much
        zero_err_epsilon = 0.2

        self.assertTrue(
            abs((expected_zero - zero) / expected_zero) < zero_err_epsilon,
            "TZERO (%f) is too far from its expected value (%f)" % (zero, expected_zero),
        )


if __name__ == "__main__":
    unittest.main()
