import unittest
from mantid.simpleapi import *
from mantid.api import *

class MSDFitTest(unittest.TestCase):

    def setUp(self):
        """
        Creates a sample workspace for testing.
        """

        sample = CreateSampleWorkspace(Function='User Defined',
                        UserDefinedFunction='name=ExpDecay,Height=1,Lifetime=6',
                        NumBanks=5, BankPixelWidth=1, XUnit='QSquared', XMin=0.0,
                        XMax=5.0, BinWidth=0.1)
        self._ws = sample


    def _validate_workspaces(self, msd_ws, param_ws, fit_ws):
        """
        Validates the various workspaces produced by MSDFit.

        @param msd_ws The MSD workspace
        @param param_ws The fit parameter table workspace
        @param fit_ws The fit workspace group
        """

        # First validate workspace types
        self.assertTrue(isinstance(msd_ws, WorkspaceGroup), 'MSD workspace should be a WorkspaceGroup')
        self.assertTrue(isinstance(param_ws, ITableWorkspace), 'Fit parameter workspace should be a TableWorkspace')
        self.assertTrue(isinstance(fit_ws, WorkspaceGroup), 'Fit workspace should be a WorkspaceGroup')

        # Validate number of items in groups
        self.assertEqual(msd_ws.getNumberOfEntries(), 2)
        self.assertEqual(fit_ws.getNumberOfEntries(), 5)

        # Validate MSD property workspaces
        self.assertEqual(msd_ws[0].name(), 'msd_A0')
        self.assertEqual(msd_ws[1].name(), 'msd_A1')
        self.assertEqual(len(msd_ws[0].readX(0)), 5)
        self.assertEqual(len(msd_ws[1].readX(0)), 5)


    def test_basic_run(self):
        """
        Tests a basic run providing the MSD workspace as output.
        """

        MSDFit(InputWorkspace=self._ws,
               XStart=0.0, XEnd=5.0,
               SpecMin=0, SpecMax=4,
               OutputWorkspace='msd')

        self.assertTrue(mtd.doesExist('msd_Parameters'), 'Should have a parameter WS with the default name')
        self.assertTrue(mtd.doesExist('msd_Workspaces'), 'Should have a fit WS with the default name')
        self._validate_workspaces(mtd['msd'], mtd['msd_Parameters'], mtd['msd_Workspaces'])


    def test_basic_run_given_names(self):
        """
        Tests a basic run providing names of all output workspaces.
        """

        msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                 XStart=0.0, XEnd=5.0,
                                 SpecMin=0, SpecMax=4)

        self._validate_workspaces(msd, param, fit)


    def test_fail_spec_min(self):
        """
        Tests validation for SpecMin >= 0.
        """

        with self.assertRaises(RuntimeError):
            msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                     XStart=0.0, XEnd=5.0,
                                     SpecMin=-1, SpecMax=0)


    def test_fail_spec_min(self):
        """
        Tests validation for SpecMin >= num histograms.
        """

        with self.assertRaises(RuntimeError):
            msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                     XStart=0.0, XEnd=5.0,
                                     SpecMin=0, SpecMax=20)


    def test_fail_spec_range(self):
        """
        Test svalidation for SpecMax >= SpecMin.
        """

        with self.assertRaises(RuntimeError):
            msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                     XStart=0.0, XEnd=5.0,
                                     SpecMin=1, SpecMax=0)


    def test_fail_x_range(self):
        """
        Tests validation for XStart < XEnd.
        """

        with self.assertRaises(RuntimeError):
            msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                     XStart=10.0, XEnd=5.0,
                                     SpecMin=0, SpecMax=0)


    def test_fail_x_range_ws(self):
        """
        Tests validation for X range in workspace range
        """

        with self.assertRaises(RuntimeError):
            msd, param, fit = MSDFit(InputWorkspace=self._ws,
                                     XStart=0.0, XEnd=20.0,
                                     SpecMin=0, SpecMax=0)


if __name__ == '__main__':
    unittest.main()
