# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import *


class EnggFitTOFFromPeaksTest(unittest.TestCase):
    def test_wrong_properties(self):
        """
        Handle in/out property issues appropriately.
        """
        # No InputWorkspace property (required)
        self.assertRaises(TypeError, EnggFitTOFFromPeaks, OutParametersTable="param_table")

        table = CreateEmptyTableWorkspace(OutputWorkspace="some_tbl_name")
        # This property doesn't belong here
        self.assertRaises(TypeError, EnggFitTOFFromPeaks, FittedPeaks=table, ExpectedPeaks="0.6, 0.9")

    def test_runs_ok_3peaks(self):
        """
        Tests fitting DIFC/TZERO on three clean peaks
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

        paramsTblName = "test_fit_dsp_tof_table"
        difa, difc, zero = EnggFitTOFFromPeaks(FittedPeaks=test_fit_peaks_table, OutParametersTable=paramsTblName)

        pTable = mtd[paramsTblName]
        self.assertEqual(pTable.rowCount(), 1)
        self.assertEqual(pTable.columnCount(), 3)

        self.assertEqual(test_fit_peaks_table.rowCount(), 3)
        self.assertEqual(3, len(test_fit_peaks_table.column("dSpacing")))
        self.assertEqual(3, len(test_fit_peaks_table.column("X0")))
        self.assertEqual(3, len(test_fit_peaks_table.column("A")))
        self.assertEqual(3, len(test_fit_peaks_table.column("S")))

        expected_difa = -5442.55806986
        self.assertTrue(self._approxRelErrorLessThan(difa, expected_difa, 5e-3))
        expected_difc = 29681.0554393
        # assertLess would be nices, but only available in unittest >= 2.7
        self.assertTrue(self._approxRelErrorLessThan(difc, expected_difc, 5e-3))
        expected_zero = -5873.54719194736
        self.assertTrue(self._approxRelErrorLessThan(zero, expected_zero, 5e-3))

    def test_4peaks_runs_ok(self):
        """
        Tests fitting DIFA/DIFC/TZERO on a four clean peaks from EnggFitPeaks.
        """

        peak_def1 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=15000, A=0.1, B=0.14, X0=15000, S=50"
        peak_def2 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=6000, A=0.02, B=0.021, X0=20000, S=40"
        peak_def3 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=10000, A=0.1, B=0.09, X0=25000, S=60"
        peak_def4 = "name=FlatBackground,A0=1;name=BackToBackExponential, I=12000, A=0.02, B=0.09, X0=30000, S=50"
        sws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction=peak_def1 + ";" + peak_def2 + ";" + peak_def3 + ";" + peak_def4,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=5000,
            XMax=40000,
            BinWidth=25,
        )
        EditInstrumentGeometry(Workspace=sws, L2=[1.5], Polar=[90], PrimaryFlightPath=50)

        peaksTblName = "test_fit_peaks_table"
        ep1 = 0.83
        ep2 = 1.09
        ep3 = 1.4
        ep4 = 1.62
        test_fit_peaks_table = EnggFitPeaks(sws, WorkspaceIndex=0, ExpectedPeaks=[ep1, ep2, ep3, ep4], OutFittedPeaksTable=peaksTblName)

        paramsTblName = "test_fit_tof_table"
        difa, difc, zero = EnggFitTOFFromPeaks(OutParametersTable=paramsTblName, FittedPeaks=test_fit_peaks_table)

        pTable = mtd[paramsTblName]
        self.assertEqual(pTable.rowCount(), 1)
        self.assertEqual(pTable.columnCount(), 3)

        self.assertEqual(test_fit_peaks_table.rowCount(), 4)

        # fitting results on some platforms (OSX) are different by ~0.07%
        # Algorithm doesn't work and is being deprecated so tolerance doesn't actually matter.
        expected_difa = 2369.8867804068127
        self.assertTrue(self._approxRelErrorLessThan(difa, expected_difa, 40))
        expected_difc = 12736.35201894777
        self.assertTrue(self._approxRelErrorLessThan(difc, expected_difc, 40))
        expected_zero = 2943.7460510348255
        self.assertTrue(self._approxRelErrorLessThan(zero, expected_zero, 40))

        # values in the table should also be good within epsilon
        self.assertTrue(self._approxRelErrorLessThan(pTable.cell(0, 1), expected_difc, 40))
        self.assertTrue(self._approxRelErrorLessThan(pTable.cell(0, 2), expected_zero, 40))

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


if __name__ == "__main__":
    unittest.main()
