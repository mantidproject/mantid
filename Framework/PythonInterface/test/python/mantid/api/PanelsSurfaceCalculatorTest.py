# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import PanelsSurfaceCalculator
from mantid.simpleapi import CreateSampleWorkspace
import numpy as np
from mantid.kernel import Quat, V3D


class PanelsSurfaceCalculatorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        super().setUpClass()
        cls._ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=5, StoreInADS=False)
        cls._calculator = PanelsSurfaceCalculator()

    def test_calculator_initialization(self):
        self.assertIsNotNone(self._calculator)

    def test_setupBasisAxes(self):
        z_axis = [0, 0, 1]
        x_axis, y_axis = [0, 0, 0], [0, 0, 0]
        self._calculator.setupBasisAxes(x_axis, y_axis, z_axis)
        self.assertEqual(x_axis, [1, 0, 0])
        self.assertEqual(y_axis, [0, 1, 0])
        self.assertEqual(z_axis, [0, 0, 1])

    def test_setupBasisAxes_ndarray(self):
        z_axis = np.array([0, 0, 1])
        x_axis, y_axis = np.array([0, 0, 0]), np.array([0, 0, 0])
        self._calculator.setupBasisAxes(x_axis, y_axis, z_axis)
        np.testing.assert_allclose([1, 0, 0], x_axis)
        np.testing.assert_allclose([0, 1, 0], y_axis)
        np.testing.assert_allclose([0, 0, 1], z_axis)

    def test_retrievePanelCorners(self):
        corners = self._calculator.retrievePanelCorners(self._ws.componentInfo(), 30)
        self.assertEqual(4, len(corners))
        absolute_tolerance = 1e-4
        np.testing.assert_allclose([0, -0.5, 5], corners[0], atol=absolute_tolerance)
        np.testing.assert_allclose([0.032, -0.5, 5], corners[1], atol=absolute_tolerance)
        np.testing.assert_allclose([0.032, 0.53205, 5], corners[2], atol=absolute_tolerance)
        np.testing.assert_allclose([0, 0.53205, 5], corners[3], atol=absolute_tolerance)

    def test_calculatePanelNormal(self):
        corners = self._calculator.retrievePanelCorners(self._ws.componentInfo(), 30)
        normal = self._calculator.calculatePanelNormal(corners)
        np.testing.assert_allclose([0, 0, 1], normal, 1e-4)

    def test_isBankFlat(self):
        normal = [0, 1, 0]
        is_bank_flat = self._calculator.isBankFlat(self._ws.componentInfo(), 25, [25], normal)
        self.assertFalse(is_bank_flat)

    def test_calculateBankNormal(self):
        normal = self._calculator.calculateBankNormal(self._ws.componentInfo(), [25, 26])
        np.testing.assert_allclose([0, 0, -1], normal, atol=1e-4)

    def test_setBankVisited(self):
        visited_components = [False for i in range(self._ws.componentInfo().size())]
        self._calculator.setBankVisited(self._ws.componentInfo(), 4, visited_components)
        self.assertTrue(visited_components[4])

    def test_findNumDetectors(self):
        components = list(range(self._ws.componentInfo().size()))
        num_detectors = self._calculator.findNumDetectors(self._ws.componentInfo(), components)
        self.assertEqual(25, num_detectors)

    def test_calcBankRotation(self):
        detector_Position = [1, 0, 0]
        normal = [0, 1, 0]
        z_axis = [0, 0, 1]
        y_axis = [0, 1, 0]
        sample_position = [0, 0, 0]
        rotation = self._calculator.calcBankRotation(detector_Position, normal, z_axis, y_axis, sample_position)
        angle = np.cos(np.pi / 4)
        expected_rotation = [angle, angle, 0, 0]
        np.testing.assert_allclose(expected_rotation, rotation, atol=1e-9)

    def test_transformedBoundingBoxPoints(self):
        quat = Quat(45, V3D(1, 0, 0))
        bounding_box_points = self._calculator.transformedBoundingBoxPoints(
            self._ws.componentInfo(), 25, [0, 0, 0], [quat.real(), quat.imagI(), quat.imagJ(), quat.imagK()], [1, 0, 0], [0, 1, 0]
        )
        point_a = bounding_box_points[0]
        point_b = bounding_box_points[1]
        tol = 1e-3
        np.testing.assert_allclose([-0.004, -3.533], point_a, atol=tol)
        np.testing.assert_allclose([0.004, -3.516], point_b, atol=tol)


if __name__ == "__main__":
    unittest.main()
