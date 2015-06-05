import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnginXFitPeaksTest(unittest.TestCase):

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
                          EnginXFitPeaks,
                          WorkspaceIndex=0, ExpectedPeaks='0.51, 0.72')

        # Wrong WorkspaceIndex value
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=-3, ExpectedPeaks='0.51, 0.72')

        # Wrong property
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          InputWorkspace=ws_name, BankPixelFoo=33,
                          WorkspaceIndex=0, ExpectedPeaks='0.51, 0.72')

        # Wrong ExpectedPeaks value
        self.assertRaises(ValueError,
                          EnginXFitPeaks,
                          InputWorkspace=ws_name,
                          WorkspaceIndex=0, ExpectedPeaks='a')


    def _check_output_ok(self, ws, ws_name='', y_dim_max=1, yvalues=None):
        """
        Checks expected output values from the fits
        """

        peak_def = "name=BackToBackExponential, I=4000,A=1, B=1.5, X0=10000, S=150"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1, XMin=5000, XMax=30000,
                                    BinWidth=5)

        # Missing input workspace
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          WorkspaceIndex=0, ExpectedPeaks=[0.0, 1.0])

        # Wrong index
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          InputWorkspace=sws,
                          WorkspaceIndex=-5, ExpectedPeaks=[0.0, 1.0])


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
                          EnginXFitPeaks,
                          sws, 0, [0.5, 2.5])
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          sws, 0, [1, 3])

        # this should fail because of nan/infinity issues
        peak_def = "name=BackToBackExponential, I=12000, A=1, B=1.5, X0=10000, S=350"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=10000, XMax=30000, BinWidth=10)
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[35], PrimaryFlightPath=35)
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
                          sws, 0, [1, 2, 3])

        # this should fail because FindPeaks doesn't initialize/converge well
        peak_def = "name=BackToBackExponential, I=90000, A=0.1, B=0.5, X0=5000, S=400"
        sws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction=peak_def,
                                    NumBanks=1, BankPixelWidth=1,
                                    XMin=2000, XMax=30000, BinWidth=10)
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(RuntimeError,
                          EnginXFitPeaks,
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
                          EnginXFitPeaks,
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
        difc, zero = EnginXFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[0.4, 1.09])
        # fitting results on some platforms (OSX) are different by ~0.07%
        expected_difc = 17395.620526173196
        self.assertTrue(abs((expected_difc-difc)/expected_difc) < 5e-3)
        expected_zero = 1058.0490117833390
        self.assertTrue(abs((expected_zero-zero)/expected_zero) < 5e-3)


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
        difc, zero = EnginXFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[0.4, 0.83, 1.09])
        expected_difc = 17335.67250113934
        # assertLess would be nices, but only available in unittest >= 2.7
        self.assertTrue(abs((expected_difc-difc)/expected_difc) < 5e-3)
        expected_zero = 958.2547157813959
        self.assertTrue(abs((expected_zero-zero)/expected_zero) < 5e-3)



if __name__ == '__main__':
    unittest.main()
