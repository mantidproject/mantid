# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import *
import mantid.simpleapi as sapi


class EnggVanadiumCorrectionsTest(unittest.TestCase):
    _data_ws = None
    _van_integ_tbl = None
    _van_curves_ws = None

    NUM_SPEC = 2513

    # Note not using @classmethod setUpClass / tearDownClass because that's not supported in the old
    # unittest of rhel6
    def setUp(self):
        """
        Set up dependencies (big files load) for one or more of the tests below.
        """
        if not self.__class__._data_ws:
            self.__class__._data_ws = sapi.LoadNexus("ENGINX00228061.nxs", OutputWorkspace="ENGIN-X_test_ws")

        if not self.__class__._van_curves_ws:
            # Note the pre-calculated file instead of the too big vanadium run
            # self.__class__._van_ws = LoadNexus("ENGINX00236516.nxs", OutputWorkspace='ENGIN-X_test_vanadium_ws')
            self.__class__._van_curves_ws = sapi.LoadNexus(
                Filename="ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs", OutputWorkspace="ENGIN-X_vanadium_curves_test_ws"
            )

        if not self.__class__._van_integ_tbl:
            self.__class__._van_integ_tbl = sapi.LoadNexus(
                Filename="ENGINX_precalculated_vanadium_run000236516_integration.nxs", OutputWorkspace="ENGIN-X_vanadium_integ_test_ws"
            )

    def test_issues_with_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required
        ones.
        """

        # absolutely wrong properties passed
        self.assertRaises(TypeError, sapi.EnggVanadiumCorrections, File="foo", Bank="1")

        # Wrong (mispelled) Workspace property
        self.assertRaises(TypeError, sapi.EnggVanadiumCorrections, InputWorkspace="anything_goes")

        # mispelled VanadiumWorkspace
        self.assertRaises(
            TypeError,
            sapi.EnggVanadiumCorrections,
            VanWorkspace=self.__class__._data_ws,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

        # mispelled CurvesWorkspace
        self.assertRaises(
            TypeError,
            sapi.EnggVanadiumCorrections,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurveWorkspace=self.__class__._van_curves_ws,
        )

        # mispelled IntegrationWorkspace
        self.assertRaises(
            TypeError,
            sapi.EnggVanadiumCorrections,
            IntegWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

        # mispelled SplineBreakPoints
        self.assertRaises(
            TypeError,
            sapi.EnggVanadiumCorrections,
            BreakPoints=self.__class__._van_integ_tbl,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

        # validation of SplineBreakPoints value bounds fails
        self.assertRaises(
            ValueError,
            sapi.EnggVanadiumCorrections,
            SplineBreakPoints=-1,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

        self.assertRaises(
            ValueError,
            sapi.EnggVanadiumCorrections,
            SplineBreakPoints=0,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

        self.assertRaises(
            ValueError,
            sapi.EnggVanadiumCorrections,
            SplineBreakPoints=3,
            IntegrationWorkspace=self.__class__._van_integ_tbl,
            CurvesWorkspace=self.__class__._van_curves_ws,
        )

    def _check_corrected_ws(self, wks):
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), "TOF")
        self.assertEqual(wks.getAxis(1).getUnit().unitID(), "Label")
        self.assertEqual(wks.getNumberHistograms(), self.NUM_SPEC)

    def _check_integ_ws(self, wks):
        self.assertTrue(isinstance(wks, ITableWorkspace), "The integration workspace should be a table workspace.")
        self.assertEqual(wks.columnCount(), 1)
        self.assertEqual(wks.rowCount(), self.NUM_SPEC)

    def _check_curves_ws(self, wks):
        self.assertEqual(0, wks.getNumberHistograms() % 3)
        self.assertTrue(isinstance(wks, MatrixWorkspace), "The integration workspace should be a matrix workspace.")

    def test_runs_ok_when_reusing_precalculated(self):
        """
        Checks normal operation, re-using previously calculated integrations and curves from
        Vanadium run data
        """
        sample_ws = self.__class__._data_ws
        int_ws = self.__class__._van_integ_tbl
        curves_ws = self.__class__._van_curves_ws
        sapi.EnggVanadiumCorrections(Workspace=sample_ws, IntegrationWorkspace=int_ws, CurvesWorkspace=curves_ws)

        self._check_corrected_ws(sample_ws)
        self._check_integ_ws(int_ws)
        self._check_curves_ws(curves_ws)

    # This is disabled because it would require loading the big vanadium run file. This is tested
    # in the EnggCalibration system test
    def disabled_test_runs_ok_when_calculating(self):
        """
        Checks normal operation, when calculating integrations and curves from Vanadium run data
        """
        sample_ws = self.__class__._data_ws
        integ_ws_name = "calc_integ_ws"
        curves_ws_name = "calc_curves_ws"
        sapi.EnggVanadiumCorrections(
            Workspace=sample_ws,
            VanadiumWorkspace=self.__class__._van_ws,
            IntegrationWorkspace=integ_ws_name,
            CurvesWorkspace=curves_ws_name,
        )

        self._check_corrected_ws(sample_ws)
        self._check_integ_ws(integ_ws_name)
        self._check_curves_ws(curves_ws_name)


if __name__ == "__main__":
    unittest.main()
