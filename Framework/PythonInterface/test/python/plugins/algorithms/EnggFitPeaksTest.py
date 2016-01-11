import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnggFitPeaksTest(unittest.TestCase):

    def test_wrong_properties(self):
        """
        Handle in/out property issues appropriately.
        """

        ws_name = 'out_ws'
        peak = "name=BackToBackExponential, I=5000,A=1, B=1., X0=10000, S=150"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak,
                                    NumBanks=1, BankPixelWidth=1, XMin=5000, XMax=30000,
                                    BinWidth=5, OutputWorkspace=ws_name)

        # No InputWorkspace property (required)
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          WorkspaceIndex=0, ExpectedPeaks='0.51, 0.72')

        # Wrong WorkspaceIndex value
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=-3, ExpectedPeaks='0.51, 0.72')

        # Wrong property
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          InputWorkspace=ws_name, BankPixelFoo=33,
                          WorkspaceIndex=0, ExpectedPeaks='0.51, 0.72')

        # Wrong ExpectedPeaks value
        self.assertRaises(ValueError,
                          EnggFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=0, ExpectedPeaks='a')


    def _approxRelErrorLessThan(self, val, ref, epsilon):
        """
        Checks that a value 'val' does not defer from a reference value 'ref' by 'epsilon'
        or more . This plays the role of assertAlmostEqual, assertLess, etc. which are not
        available in some ancient unittest versions.

        @param val :: value obtained from a calculation or algorithm
        @param ref :: (expected) reference value
        @param epsilon :: comparison epsilon (error tolerance)

        @returns if val differs from ref by less than epsilon
        """
        if 0 == ref:
            return False
        return (abs((ref-val)/ref) < epsilon)

    def _check_outputs_ok(self, tblName, numPeaks, cell00, cell01, cell10, cell14):
        """
        Checks that we get the expected types and values in the outputs.

        @param tblName :: name of the table of peaks that should have been created
        @param numPeaks :: number of peaks that should be found in the table
        @param cell00 :: expected (good) value for cell(0,0)
        @param cell11 :: expected (good) value for cell(0,1)
        @param cell11 :: expected (good) value for cell(1,0)
        @param cell14 :: expected (good) value for cell(1,4)
        """

        # it has ben created
        tbl = mtd[tblName]
        self.assertEquals(tbl.getName(), tblName)
        self.assertTrue(isinstance(tbl, ITableWorkspace),
                        'The output workspace of fitted peaks should be a table workspace.')

        # number of peaks
        self.assertEquals(tbl.rowCount(), numPeaks)
        # number of parameters for every peak
        colNames = ['dSpacing',
                    'A0', 'A0_Err', 'A1', 'A1_Err', 'X0', 'X0_Err', 'A', 'A_Err',
                    'B', 'B_Err', 'S', 'S_Err', 'I', 'I_Err',
                    'Chi']
        self.assertEquals(tbl.columnCount(), len(colNames))

        # expected columns
        self.assertEquals(tbl.getColumnNames(), colNames)

        # some values
        # note approx comparison - fitting results differences of ~5% between glinux/win/osx
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(0,0), cell00, 5e-3))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(0,1), cell01, 5e-3))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(1,0), cell10, 5e-3))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(1,4), cell14, 5e-3))

    def test_fitting_fails_ok(self):
        """
        Tests acceptable response (appropriate exception) when fitting doesn't work well
        """

        peak_def = "name=BackToBackExponential, I=8000, A=1, B=1.2, X0=10000, S=150"
        sws = CreateSampleWorkspace(Function="User Defined",
                                    UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=10000, XMax=30000, BinWidth=10, Random=1)
        # these should raise because of issues with the peak center - data range
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          sws, 0, [0.5, 2.5])
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          sws, 0, [1.1, 3])

        # this should fail because of nan/infinity issues
        peak_def = "name=BackToBackExponential, I=12000, A=1, B=1.5, X0=10000, S=350"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=10000, XMax=30000, BinWidth=10)
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[35], PrimaryFlightPath=35)
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          sws, 0, [1, 2.3, 3])

        # this should fail because FindPeaks doesn't initialize/converge well
        peak_def = "name=BackToBackExponential, I=90000, A=0.1, B=0.5, X0=5000, S=400"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=2000, XMax=30000, BinWidth=10)
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          sws, 0, [0.6])

    def test_fails_ok_1peak(self):
        """
        Tests fitting a single peak, which should raise because we need at least 2 peaks to
        fit two parameters: difc and zero
        """
        peak_def = "name=BackToBackExponential, I=15000, A=1, B=1.2, X0=10000, S=400"
        sws = CreateSampleWorkspace(Function="User Defined",
                                    UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=0, XMax=25000, BinWidth=10)
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=45)
        self.assertRaises(RuntimeError,
                          EnggFitPeaks,
                          sws, WorkspaceIndex=0, ExpectedPeaks='0.542')

    def test_runs_ok_2peaks(self):
        """
        Tests fitting a couple of peaks.
        """

        peak_def1 = "name=BackToBackExponential, I=10000, A=1, B=0.5, X0=8000, S=350"
        peak_def2 = "name=BackToBackExponential, I=8000, A=1, B=1.7, X0=20000, S=300"
        sws = CreateSampleWorkspace(Function="User Defined",
                                    UserDefinedFunction=peak_def1 + ";" + peak_def2,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=5000, XMax=30000,
                                    BinWidth=25)
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = 'test_fit_peaks_table'
        ep1 = 0.4
        ep2 = 1.09
        paramsTblName = 'test_difc_zero_table'
        difc, zero = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2],
                                    OutFittedPeaksTable=peaksTblName,
                                    OutParametersTable=paramsTblName)
        pTable = mtd[paramsTblName]
        self.assertEquals(pTable.rowCount(), 1)
        self.assertEquals(pTable.columnCount(), 2)

        # fitting results on some platforms (OSX) are different by ~0.07%
        expected_difc = 17395.620526173196
        self.assertTrue(self._approxRelErrorLessThan(difc, expected_difc, 5e-3))
        expected_zero = 1058.0490117833390
        self.assertTrue(self._approxRelErrorLessThan(zero, expected_zero, 5e-3))

        # values in the table should also be good within epsilon
        self.assertTrue(self._approxRelErrorLessThan(pTable.cell(0,0), expected_difc, 5e-3))
        self.assertTrue(self._approxRelErrorLessThan(pTable.cell(0,1), expected_zero, 5e-3))

        # check 'OutFittedPeaksTable' table workspace
        self._check_outputs_ok(peaksTblName, 2, ep1, -8.193560670563205e-10,
                               ep2, 1.723902507582676e-07)

    def test_runs_ok_3peaks(self):
        """
        Tests fitting three clean peaks and different widths.
        """

        peak_def1 = "name=BackToBackExponential, I=10000, A=1, B=0.5, X0=8000, S=350"
        peak_def2 = "name=BackToBackExponential, I=15000, A=1, B=1.7, X0=15000, S=100"
        peak_def3 = "name=BackToBackExponential, I=8000, A=1, B=1.2, X0=20000, S=800"
        sws = CreateSampleWorkspace(Function="User Defined",
                                    UserDefinedFunction=
                                    peak_def1 + ";" + peak_def2 + ";" + peak_def3,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=5000, XMax=30000,
                                    BinWidth=25)
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = 'test_fit_peaks_table'
        ep1 = 0.4
        ep2 = 0.83
        ep3 = 1.09
        difc, zero = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2, ep3],
                                    OutFittedPeaksTable=peaksTblName)

        expected_difc = 17335.67250113934
        # assertLess would be nices, but only available in unittest >= 2.7
        self.assertTrue(self._approxRelErrorLessThan(difc, expected_difc, 5e-3))
        expected_zero = 958.2547157813959
        self.assertTrue(self._approxRelErrorLessThan(zero, expected_zero, 5e-3))

        # check 'OutFittedPeaksTable' table workspace
        self._check_outputs_ok(peaksTblName, 3, ep1, -8.193560670563205e-10,
                               ep2, 1.3304856478614542e-05)


if __name__ == '__main__':
    unittest.main()
