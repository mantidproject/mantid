# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AlgorithmManager, FrameworkManager
from sans_core.algorithm_detail.strip_end_nans_and_infs import strip_end_nans


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
        self.assertEqual(len(data_y), 5)
        self.assertEqual(data_y[0], 36.0)
        self.assertEqual(data_y[1], 44.0)
        self.assertEqual(data_y[2], 52.0)
        self.assertEqual(data_y[3], 63.0)
        self.assertEqual(data_y[4], 75.0)

    def test_that_can_strip_end_nans_and_infs_for_point_workspace(self):
        data_x = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        data_y = [float("Nan"), float("Inf"), 36.0, 44.0, 52.0, 63.0, 75.0, float("Inf"), float("Nan"), float("Inf")]
        self._do_test(data_x, data_y)

    def test_that_can_strip_end_nans_and_infs_for_histo_workspace(self):
        data_x = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        data_y = [float("Nan"), float("Inf"), 36.0, 44.0, 52.0, 63.0, 75.0, float("Inf"), float("Nan"), float("Inf")]
        self._do_test(data_x, data_y)


if __name__ == "__main__":
    unittest.main()
