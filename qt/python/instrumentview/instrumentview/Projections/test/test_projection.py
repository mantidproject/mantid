# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest.mock
from instrumentview.Projections.cylindrical_projection import cylindrical_projection

import numpy as np
import unittest
from unittest.mock import MagicMock


class TestProjection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_workspace = MagicMock()
        cls.mock_componentInfo = MagicMock()
        cls.mock_componentInfo.samplePosition.return_value = np.array([0, 0, 0])
        cls.mock_componentInfo.position = cls.mock_position
        cls.mock_componentInfo.hasValidShape.return_value = True
        cls.mock_workspace.componentInfo.return_value = cls.mock_componentInfo
        cls.detector_indices = [0, 1, 2]

    def mock_position(index: int):
        if index == 0:
            return [0, 1, 0]
        if index == 1:
            return [2, 1, 0]
        if index == 2:
            return [-2, 1, 0]

        raise ValueError(f"Unexpected component index: {index}")

    def test_apply_x_correction_below_min(self):
        proj = cylindrical_projection(self.mock_workspace, self.detector_indices, [0, 1, 0])
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_min - np.pi / 2
        self.assertLess(proj._detector_x_coordinates[0], x_min)
        proj._apply_x_correction(0)
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    def test_apply_x_correction_above_max(self):
        proj = cylindrical_projection(self.mock_workspace, self.detector_indices, [0, 1, 0])
        x_min = proj._x_range[0]
        x_max = proj._x_range[1]
        proj._detector_x_coordinates[0] = x_max + np.pi / 2
        self.assertGreater(proj._detector_x_coordinates[0], x_max)
        proj._apply_x_correction(0)
        self.assertGreaterEqual(proj._detector_x_coordinates[0], x_min)
        self.assertLessEqual(proj._detector_x_coordinates[0], x_max)

    @unittest.mock.patch("instrumentview.Projections.projection.projection._apply_x_correction")
    def test_find_and_correct_x_gap(self, mock_apply_x_correction):
        proj = cylindrical_projection(self.mock_workspace, self.detector_indices, [0, 0, 1])
        proj._u_period = (proj._x_range[1] - proj._x_range[0]) / 2
        mock_apply_x_correction.reset_mock()
        proj._find_and_correct_x_gap()
        self.assertEqual(len(self.detector_indices), mock_apply_x_correction.call_count)
