# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, NormaliseSpectra


class NormaliseSpectraTest(unittest.TestCase):
    _positive = "1,2,3,4,5"
    _negative = "-5,-4,-3,-2,-1"
    _zeros = "0,0,0,0,0"
    _mixed = "-2,-1,0,1,2"

    # ----------------------------------Algorithm tests----------------------------------------

    def test_one_hist_positive(self):
        in_ws = self._create_workspace(1, "test", self._positive)
        out_ws = NormaliseSpectra(InputWorkspace=in_ws)
        self._check_workspace_is_within_boundaries(out_ws, 1, 0)

    def test_one_hist_negative(self):
        in_ws = self._create_workspace(1, "test", self._negative)
        self.assertRaisesRegex(
            RuntimeError,
            "Spectrum number 1: has a maximum y value of 0 or less. All spectra must have a maximum y value more than 0",
            NormaliseSpectra,
            InputWorkspace=in_ws,
            OutputWorkspace="out_ws",
        )

    def test_one_hist_zeros(self):
        in_ws = self._create_workspace(1, "test", self._zeros)
        self.assertRaisesRegex(
            RuntimeError,
            "Spectrum number 1: has a maximum y value of 0 or less. All spectra must have a maximum y value more than 0",
            NormaliseSpectra,
            InputWorkspace=in_ws,
            OutputWorkspace="out_ws",
        )

    def test_one_hist_mixed(self):
        in_ws = self._create_workspace(1, "test", self._mixed)
        out_ws = NormaliseSpectra(InputWorkspace=in_ws)
        self._check_workspace_is_within_boundaries(out_ws, 1, -1)

    def test_with_MatrixWorkspace_multi_hist_(self):
        in_ws = self._create_workspace(3, "test", self._positive)
        out_ws = NormaliseSpectra(InputWorkspace=in_ws)
        self._check_workspace_is_within_boundaries(out_ws, 1, 0)

    def test_with_nan(self):
        in_ws = self._create_workspace(1, "test", "nan,2,3,4,5")
        out_ws = NormaliseSpectra(InputWorkspace=in_ws)
        self._check_workspace_is_within_boundaries(out_ws, 1, 0)

    # --------------------------------Validate results-----------------------------------------

    def _check_workspace_is_within_boundaries(self, matrixWs, max, min):
        for i in range(matrixWs.getNumberHistograms()):
            self._check_spectrum_less_than(matrixWs.readY(i), max)
            self._check_spectrum_more_than(matrixWs.readY(i), min)

    def _check_spectrum_less_than(self, y_data, upper_boundary):
        for i in range(len(y_data)):
            if numpy.isnan(y_data[i]):
                continue
            self.assertLessEqual(y_data[i], upper_boundary)

    def _check_spectrum_more_than(self, y_data, lower_boundary):
        for i in range(len(y_data)):
            if numpy.isnan(y_data[i]):
                continue
            self.assertGreaterEqual(y_data[i], lower_boundary)

    # --------------------------------Helper Functions-----------------------------------------
    def _create_workspace(self, nhists, out_name, data_string):
        """
        Creates a basic Matrixworkspace
        """
        data = ""
        for i in range(nhists):
            data += data_string
            if i != (nhists - 1):
                data += ","
        CreateWorkspace(OutputWorkspace=out_name, DataX=data, DataY=data, DataE=data, Nspec=nhists)
        return mtd[out_name]


if __name__ == "__main__":
    unittest.main()
