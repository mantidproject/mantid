# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest

from mantid.api import AlgorithmManager, FrameworkManager
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans


class StripEndNansTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def _do_test(self, data_x, data_y):
        # Arrange
        alg_ws = AlgorithmManager.createUnmanaged("CreateWorkspace")
        alg_ws.setChild(True)
        alg_ws.initialize()
        alg_ws.setProperty("OutputWorkspace", "test")

        alg_ws.setProperty("DataX", data_x)
        alg_ws.setProperty("DataY", data_y)
        alg_ws.execute()
        workspace = alg_ws.getProperty("OutputWorkspace").value

        # Act
        cropped_workspace = strip_end_nans(workspace)
        # Assert
        data_y = cropped_workspace.dataY(0)
        self.assertEqual(len(data_y),  5)
        self.assertEqual(data_y[0],  36.)
        self.assertEqual(data_y[1],  44.)
        self.assertEqual(data_y[2],  52.)
        self.assertEqual(data_y[3],  63.)
        self.assertEqual(data_y[4],  75.)

    def test_that_can_strip_end_nans_and_infs_for_point_workspace(self):
        data_x = [1., 2., 3., 4., 5., 6., 7., 8., 9., 10.]
        data_y = [float("Nan"), float("Inf"), 36., 44., 52., 63., 75., float("Inf"), float("Nan"), float("Inf")]
        self._do_test(data_x, data_y)

    def test_that_can_strip_end_nans_and_infs_for_histo_workspace(self):
        data_x = [1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11.]
        data_y = [float("Nan"), float("Inf"), 36., 44., 52., 63., 75., float("Inf"), float("Nan"), float("Inf")]
        self._do_test(data_x, data_y)


if __name__ == '__main__':
    unittest.main()
