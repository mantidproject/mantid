# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, ITableWorkspace
from mantid.simpleapi import CreateSampleWorkspace, EditInstrumentGeometry, EnggFitPeaks


class EnggFitPeaksTest(unittest.TestCase):
    def test_wrong_properties(self):
        """
        Handle in/out property issues appropriately.
        """

        ws_name = "out_ws"
        peak = "name=BackToBackExponential, I=5000,A=1, B=1., X0=10000, S=150"
        CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=5000,
            XMax=30000,
            BinWidth=5,
            OutputWorkspace=ws_name,
        )

        # No InputWorkspace property (required)
        self.assertRaises(TypeError, EnggFitPeaks, WorkspaceIndex=0, ExpectedPeaks="0.51, 0.72")

        # Wrong WorkspaceIndex value
        self.assertRaises(TypeError, EnggFitPeaks, InputWorkspace=ws_name, WorkspaceIndex=-3, ExpectedPeaks="0.51, 0.72")

        # Wrong property
        self.assertRaises(TypeError, EnggFitPeaks, InputWorkspace=ws_name, BankPixelFoo=33, WorkspaceIndex=0, ExpectedPeaks="0.51, 0.72")

        # missing FittedPeaks output property
        self.assertRaises(TypeError, EnggFitPeaks, InputWorkspace=ws_name, WorkspaceIndex=0, ExpectedPeaks="0.51, 0.72")

        # Wrong ExpectedPeaks value
        self.assertRaises(ValueError, EnggFitPeaks, InputWorkspace=ws_name, WorkspaceIndex=0, ExpectedPeaks="a")

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

        approx_comp = abs((ref - val) / ref) < epsilon
        if not approx_comp:
            print("Failed approximate comparison between value {0} and reference value " "{1}, with epsilon {2}".format(val, ref, epsilon))

        return approx_comp

    def _check_outputs_ok(self, tbl_name, num_peaks, cells):
        """
        Checks that we get the expected types and values in the outputs.

        @param tbl_name :: name of the table of peaks that should have been created
        @param num_peaks :: number of peaks that should be found in the table
        @param cells :: list of expected (good) values for several cells:
        cell(0,0), cell(0,1), cell(1,0), cell(1,4)
        """

        cell00, cell01, cell10, cell14 = cells
        # it has ben created
        tbl = mtd[tbl_name]
        self.assertEqual(tbl.name(), tbl_name)
        self.assertTrue(isinstance(tbl, ITableWorkspace), "The output workspace of fitted peaks should be a table workspace.")

        # number of peaks
        self.assertEqual(tbl.rowCount(), num_peaks)
        # number of parameters for every peak
        col_names = [
            "dSpacing",
            "A0",
            "A0_Err",
            "A1",
            "A1_Err",
            "X0",
            "X0_Err",
            "A",
            "A_Err",
            "B",
            "B_Err",
            "S",
            "S_Err",
            "I",
            "I_Err",
            "Chi",
        ]
        self.assertEqual(tbl.columnCount(), len(col_names))

        # expected columns
        self.assertEqual(tbl.getColumnNames(), col_names)

        # some values
        # note approx comparison - fitting results differences of ~5% between glinux/win/osx
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(0, 0), cell00, 5e-2))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(0, 1), cell01, 5e-2))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(1, 0), cell10, 5e-2))
        self.assertTrue(self._approxRelErrorLessThan(tbl.cell(1, 4), cell14, 5e-2))

    def test_fitting_fails_ok(self):
        """
        Tests acceptable response (appropriate exception) when fitting doesn't work well
        """

        peak_def = "name=BackToBackExponential, I=8000, A=1, B=1.2, X0=10000, S=150"
        sws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak_def,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=10000,
            XMax=30000,
            BinWidth=10,
            Random=1,
        )
        # these should raise because of issues with the peak center - data range
        self.assertRaises(TypeError, EnggFitPeaks, sws, 0, [0.5, 2.5])
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(TypeError, EnggFitPeaks, sws, 0, [1.1, 3])

        # this should fail because of nan/infinity issues
        peak_def = "name=BackToBackExponential, I=12000, A=1, B=1.5, X0=10000, S=350"
        sws = CreateSampleWorkspace(
            Function="User Defined", UserDefinedFunction=peak_def, NumBanks=1, BankPixelWidth=1, XMin=10000, XMax=30000, BinWidth=10
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[35], PrimaryFlightPath=35)
        self.assertRaises(TypeError, EnggFitPeaks, sws, 0, [1, 2.3, 3])

        # this should fail because FindPeaks doesn't initialize/converge well
        peak_def = "name=BackToBackExponential, I=90000, A=0.1, B=0.5, X0=5000, S=400"
        sws = CreateSampleWorkspace(
            Function="User Defined", UserDefinedFunction=peak_def, NumBanks=1, BankPixelWidth=1, XMin=2000, XMax=30000, BinWidth=10
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.0], Polar=[90], PrimaryFlightPath=50)
        self.assertRaises(TypeError, EnggFitPeaks, sws, 0, [0.6])

    def test_fails_ok_1peak(self):
        """
        Tests fitting a single peak, which should raise
        """
        peak_def = "name=BackToBackExponential, I=15000, A=1, B=1.2, X0=10000, S=400"
        sws = CreateSampleWorkspace(
            Function="User Defined", UserDefinedFunction=peak_def, NumBanks=1, BankPixelWidth=1, XMin=0, XMax=25000, BinWidth=10
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=45)
        self.assertRaises(TypeError, EnggFitPeaks, sws, WorkspaceIndex=0, ExpectedPeaks="0.542")

    def test_2peaks_fails_1(self):
        """
        Tests fitting a couple of peaks where Fit fails to fit with enough
        accuracy one of them.
        """

        peak_def1 = "name=BackToBackExponential, I=10000, A=1, B=0.5, X0=8000, S=350"
        peak_def2 = "name=BackToBackExponential, I=1000, A=1, B=1.7, X0=20000, S=300"
        sws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak_def1 + ";" + peak_def2,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=5000,
            XMax=30000,
            BinWidth=25,
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = "test_fit_peaks_table"
        ep1 = 0.4
        ep2 = 1.09

        # will fail to fit the first peak (too far off initial guess)
        try:
            test_fit_peaks_table = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2], OutFittedPeaksTable=peaksTblName)
            self.assertEqual(test_fit_peaks_table.rowCount(), 1)
        except RuntimeError as rex:
            print("Failed (as expected) to fit the first peak (too far off the initial " "guess), with RuntimeError: {0}".format(str(rex)))

    def test_2peaks_runs_ok(self):
        """
        Tests fitting a couple of peaks.
        """

        peak_def1 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=15000, A=0.1, B=0.14, X0=15000, S=50"
        peak_def2 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=6000, A=0.02, B=0.021, X0=20000, S=40"
        sws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak_def1 + ";" + peak_def2,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=5000,
            XMax=30000,
            BinWidth=25,
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = "test_fit_peaks_table"
        ep1 = 0.83
        ep2 = 1.09
        test_fit_peaks_table = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2], OutFittedPeaksTable=peaksTblName)

        self.assertEqual(test_fit_peaks_table.rowCount(), 2)

        # check 'OutFittedPeaksTable' table workspace
        self._check_outputs_ok(peaksTblName, 2, [ep1, 1.98624796464, ep2, 0.00167306637639])

    def test_runs_ok_3peaks(self):
        """
        Tests fitting three clean peaks and different widths.
        """

        peak_def1 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=15000, A=0.1, B=0.14, X0=15000, S=50"
        peak_def2 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=6000, A=0.02, B=0.021, X0=20000, S=40"
        peak_def3 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=10000, A=0.1, B=0.09, X0=25000, S=60"
        sws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak_def1 + ";" + peak_def2 + ";" + peak_def3,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=5000,
            XMax=30000,
            BinWidth=25,
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = "test_fit_peaks_table"
        ep1 = 0.83
        ep2 = 1.09
        ep3 = 1.4
        test_fit_peaks_table = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2, ep3], OutFittedPeaksTable=peaksTblName)

        self.assertEqual(test_fit_peaks_table.rowCount(), 3)
        self.assertEqual(3, len(test_fit_peaks_table.column("dSpacing")))
        self.assertEqual(3, len(test_fit_peaks_table.column("X0")))
        self.assertEqual(3, len(test_fit_peaks_table.column("A")))
        self.assertEqual(3, len(test_fit_peaks_table.column("S")))

        # check 'OutFittedPeaksTable' table workspace
        self._check_outputs_ok(peaksTblName, 3, [ep1, 2.98294345043, ep2, 0.00197567235850])

    def test_reject_outlying_peaks(self):
        """
        Tests that if all peaks lie outside the limits of the x axis of the input workspace, the algorithm fails
        """
        input_ws = CreateSampleWorkspace(XUnit="TOF", XMin=5000, XMax=30000)
        error_msg = None

        try:
            EnggFitPeaks(InputWorkspace=input_ws, WorkspaceIndex=0, ExpectedPeaks=[35000.0, 40000.0], FittedPeaks="peaks")
        except RuntimeError as e:
            error_msg = e.args[0].split("\n")[0]

        self.assertEqual(error_msg, "EnggFitPeaks-v1: Expected peak centres lie outside the limits of the workspace x axis")

    def test_expected_peaks_can_be_in_tof(self):
        """
        Tests that the expected peak values can be given in TOF - they should just be converted
        """
        peak_def1 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=15000, A=0.1, B=0.14, X0={}, S=50"
        peak_def2 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=6000, A=0.02, B=0.021, X0={}, S=40"
        peak_centre_1_tof = 41209
        peak_centre_2_tof = 46895
        peaks_def = peak_def1.format(peak_centre_1_tof) + ";" + peak_def2.format(peak_centre_2_tof)
        sample_ws = CreateSampleWorkspace(
            Function="User Defined", UserDefinedFunction=peaks_def, NumBanks=1, BankPixelWidth=1, XMin=40000, XMax=50000, BinWidth=25
        )

        EditInstrumentGeometry(Workspace=sample_ws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)
        fit_results = EnggFitPeaks(InputWorkspace=sample_ws, WorkspaceIndex=0, ExpectedPeaks=[peak_centre_1_tof, peak_centre_2_tof])

        peak_centre_1_d = 2.238360
        peak_centre_2_d = 2.547208

        self.assertAlmostEqual(fit_results.cell(0, 0), peak_centre_1_d, 6)
        self.assertAlmostEqual(fit_results.cell(1, 0), peak_centre_2_d, 6)

    def test_expected_peaks_gsas_conversion_t_d(self):
        peak_def1 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=15000, A=0.1, B=0.14, X0={}, S=50"
        peak_def2 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=6000, A=0.02, B=0.021, X0={}, S=40"
        peak_centre_1_tof = 41209
        peak_centre_2_tof = 46895
        peaks_def = peak_def1.format(peak_centre_1_tof) + ";" + peak_def2.format(peak_centre_2_tof)
        sample_ws = CreateSampleWorkspace(
            Function="User Defined", UserDefinedFunction=peaks_def, NumBanks=1, BankPixelWidth=1, XMin=40000, XMax=50000, BinWidth=25
        )
        run = sample_ws.mutableRun()
        run.addProperty("difa", 0.0, False)
        run.addProperty("difc", 18400.0, False)
        run.addProperty("tzero", 4.0, False)

        EditInstrumentGeometry(Workspace=sample_ws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)
        fit_results = EnggFitPeaks(InputWorkspace=sample_ws, WorkspaceIndex=0, ExpectedPeaks=[peak_centre_1_tof, peak_centre_2_tof])
        peak_centre_1_d = 2.239402
        peak_centre_2_d = 2.548424

        self.assertAlmostEqual(fit_results.cell(0, 0), peak_centre_1_d, 6)
        self.assertAlmostEqual(fit_results.cell(1, 0), peak_centre_2_d, 6)


if __name__ == "__main__":
    unittest.main()
