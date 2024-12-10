# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, ITableWorkspace, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, MSDFit


class MSDFitTest(unittest.TestCase):
    def setUp(self):
        """
        Creates a sample workspace for testing.
        """

        sample = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=ExpDecay,Height=1,Lifetime=6",
            NumBanks=5,
            BankPixelWidth=1,
            XUnit="QSquared",
            XMin=0.0,
            XMax=5.0,
            BinWidth=0.1,
        )
        self._ws = sample

    def _validate_workspaces(self, msd_ws, param_ws, fit_ws):
        """
        Validates the various workspaces produced by MSDFit.

        @param msd_ws The MSD workspace
        @param param_ws The fit parameter table workspace
        @param fit_ws The fit workspace group
        """

        # First validate workspace types
        self.assertTrue(isinstance(msd_ws, MatrixWorkspace), "MSD workspace should be a MatrixWorkspace")
        self.assertTrue(isinstance(param_ws, ITableWorkspace), "Fit parameter workspace should be a TableWorkspace")
        self.assertTrue(isinstance(fit_ws, WorkspaceGroup), "Fit workspace should be a WorkspaceGroup")

        # Validate number of items in groups
        self.assertEqual(fit_ws.getNumberOfEntries(), 5)

        # Validate MSD property workspaces
        self.assertEqual(len(msd_ws.readX(0)), 5)
        self.assertEqual(len(msd_ws.readX(1)), 5)

    def test_basic_run(self):
        """
        Tests a basic run providing the MSD workspace as output.
        """

        MSDFit(InputWorkspace=self._ws, XStart=0.0, XEnd=5.0, SpecMin=0, SpecMax=4, OutputWorkspace="msd")

        self.assertTrue(mtd.doesExist("msd_Parameters"), "Should have a parameter WS with the default name")
        self.assertTrue(mtd.doesExist("msd_Workspaces"), "Should have a fit WS with the default name")
        self._validate_workspaces(mtd["msd"], mtd["msd_Parameters"], mtd["msd_Workspaces"])

    def test_basic_run_given_names(self):
        """
        Tests a basic run providing names of all output workspaces.
        """

        msd, param, fit = MSDFit(InputWorkspace=self._ws, XStart=0.0, XEnd=5.0, SpecMin=0, SpecMax=4)

        self._validate_workspaces(msd, param, fit)

    def test_fail_spec_min(self):
        """
        Tests validation for SpecMin >= 0.
        """

        with self.assertRaisesRegex(RuntimeError, "Minimum spectrum number must be greater than or equal to 0"):
            msd, param, fit = MSDFit(InputWorkspace=self._ws, XStart=0.0, XEnd=5.0, SpecMin=-1, SpecMax=0)

    def test_fail_spec_max(self):
        """
        Tests validation for SpecMax >= num histograms.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Maximum spectrum number must be less than number of spectra in workspace",
            MSDFit,
            InputWorkspace=self._ws,
            XStart=0.0,
            XEnd=5.0,
            SpecMin=0,
            SpecMax=20,
            OutputWorkspace="msd",
        )

    def test_fail_spec_range(self):
        """
        Tests validation for SpecMin >= SpecMax.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "SpecMin must be less than SpecMax",
            MSDFit,
            InputWorkspace=self._ws,
            XStart=0.0,
            XEnd=5.0,
            SpecMin=1,
            SpecMax=0,
            OutputWorkspace="msd",
        )

    def test_fail_x_range(self):
        """
        Tests validation for XStart > XEnd.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "XStart must be less than XEnd",
            MSDFit,
            InputWorkspace=self._ws,
            XStart=10.0,
            XEnd=5.0,
            SpecMin=0,
            SpecMax=0,
            OutputWorkspace="msd",
        )

    def test_fail_x_range_ws(self):
        """
        Tests validation for X range in workspace range
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Must be less than maximum X value in workspace",
            MSDFit,
            InputWorkspace=self._ws,
            XStart=0.0,
            XEnd=20.0,
            SpecMin=0,
            SpecMax=0,
            OutputWorkspace="msd",
        )


if __name__ == "__main__":
    unittest.main()
