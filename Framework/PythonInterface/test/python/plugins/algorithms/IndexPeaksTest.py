# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    IndexPeaks,
    AnalysisDataService,
    LoadEmptyInstrument,
    PredictPeaks,
    SetUB,
    PredictSatellitePeaks,
    CombinePeaksWorkspaces,
)
import unittest
import numpy as np


class IndexPeaksTest(unittest.TestCase):
    """
    The purpose of the testing is to ensure the Python binding works
    with various different input arguments.
    """

    def setUp(self):
        # load empty instrument so can create a peak table
        self.ws = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="sxd")
        ub = np.array([[-0.00601763, 0.07397297, 0.05865706], [0.05373321, 0.050198, -0.05651455], [-0.07822144, 0.0295911, -0.04489172]])
        SetUB(self.ws, UB=ub)
        PredictPeaks(self.ws, WavelengthMin=1, WavelengthMax=1.1, MinDSpacing=1, MaxDSPacing=1.1, OutputWorkspace="test")  # 8 peaks
        PredictSatellitePeaks(Peaks="test", SatellitePeaks="test_sat", ModVector1="0,0,0.33", MaxOrder=1)
        self.peaks = CombinePeaksWorkspaces(LHSWorkspace="test_sat", RHSWorkspace="test", OutputWorkspace="test")

    def tearDown(self):
        AnalysisDataService.clear()

    def test_exec_with_default_args(self):
        index_output = IndexPeaks(PeaksWorkspace="test", Tolerance=0.12)
        self.assertEqual(index_output[0], 31)

    def test_exec_with_mod_vector_supplied_correctly(self):
        index_output = IndexPeaks(
            PeaksWorkspace="test", Tolerance=0.12, RoundHKLs=False, SaveModulationInfo=True, MaxOrder=1, ModVector1="0,0,0.33333"
        )
        self.assertEqual(index_output[0], 31)

    def test_exec_throws_error_saving_mod_info_when_no_mod_vector_supplied(self):
        with self.assertRaisesRegex(RuntimeError, "SaveModulationInfo"):
            # testing zero (default) modVec with maxOrder=1
            IndexPeaks(
                PeaksWorkspace="test",
                Tolerance=0.12,
                RoundHKLs=False,
                SaveModulationInfo=True,
                MaxOrder=1,
            )

    def test_exec_throws_error_saving_mod_info_when_zero_max_order(self):
        with self.assertRaisesRegex(RuntimeError, "MaxOrder"):
            IndexPeaks(
                PeaksWorkspace="test",
                Tolerance=0.12,
                RoundHKLs=False,
                SaveModulationInfo=True,
                ModVector1="0,0,0.33333",
            )  # MaxOrder=0 by default


if __name__ == "__main__":
    unittest.main()
