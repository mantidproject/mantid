# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace


class SwapWidthsTest(unittest.TestCase):
    _input_ws = "IN16B_125878_QLd_Result"
    _swap_point = 5

    def setUp(self):
        self._input_ws = Load(Filename="IN16B_125878_QLd_Result.nxs", OutputWorkspace=self._input_ws)

    def _validate_result_shape(self, result):
        """
        Validates that the result workspace is of the correct type, units and shape.

        @param result Result workspace from SwapWidths algorithm
        """

        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEqual(result.getNumberHistograms(), 2)
        self.assertEqual(result.blocksize(), self._input_ws.blocksize())
        self.assertEqual(result.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(result.getAxis(1).label(0), "f2.f1.FWHM")
        self.assertEqual(result.getAxis(1).label(1), "f2.f2.FWHM")

    def _validate_result_values(self, result):
        """
        Validates the result workspace has the expected values to a realistic number of significant figures.

        @param result :: The Result workspace from SwapWidths algorithm
        """

        # Get f2.f1/2.FWHM axis numbers
        # f2.f1.FWHM = first_fwhm, f2.f2.FWHM = second_fwhm
        first_fwhm_idx = 0
        second_fwhm_idx = 0
        for i in range(0, self._input_ws.getNumberHistograms() - 1):
            if self._input_ws.getAxis(1).label(i) == "f2.f1.FWHM":
                first_fwhm_idx = i
            if self._input_ws.getAxis(1).label(i) == "f2.f2.FWHM":
                second_fwhm_idx = i

        # Get Y Data for input/result
        in_first_fwhm = self._input_ws.dataY(first_fwhm_idx)
        in_second_fwhm = self._input_ws.dataY(second_fwhm_idx)
        result_first_fwhm = result.dataY(0)
        result_second_fwhm = result.dataY(1)

        # Check data is correct after swap
        for i in range(0, len(in_first_fwhm)):
            if i <= self._swap_point:
                self.assertEqual(in_first_fwhm[i], result_first_fwhm[i])
                self.assertEqual(in_second_fwhm[i], result_second_fwhm[i])
            else:
                self.assertEqual(in_first_fwhm[i], result_second_fwhm[i])
                self.assertEqual(in_second_fwhm[i], result_first_fwhm[i])

    def test_basic(self):
        """
        Tests a basic run of SwapWidths.
        """
        result = SwapWidths(InputWorkspace=self._input_ws, SwapPoint=self._swap_point)
        self._validate_result_shape(result)
        self._validate_result_values(result)


if __name__ == "__main__":
    unittest.main()
