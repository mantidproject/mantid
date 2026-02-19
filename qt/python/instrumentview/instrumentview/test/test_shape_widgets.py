# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
import numpy as np

from instrumentview.ShapeWidgets import (
    ImplicitEllipse,
    CylinderWidgetNoRotation,
    RectangleWidgetNoRotation,
    EllipseWidgetNoRotation,
)


class TestImplicitEllipse(unittest.TestCase):
    def setUp(self):
        self.ellipse = ImplicitEllipse()

    def test_init(self):
        self.assertEqual(self.ellipse.cx, 0.0)
        self.assertEqual(self.ellipse.cy, 0.0)
        self.assertEqual(self.ellipse.rx, 1.0)
        self.assertEqual(self.ellipse.ry, 1.0)
        self.assertEqual(self.ellipse.angle, 0.0)

    def test_set_center(self):
        self.ellipse.set_center(5.0, 3.0)
        self.assertEqual(self.ellipse.cx, 5.0)
        self.assertEqual(self.ellipse.cy, 3.0)

    def test_set_radii(self):
        self.ellipse.set_radii(2.0, 1.5)
        self.assertEqual(self.ellipse.rx, 2.0)
        self.assertEqual(self.ellipse.ry, 1.5)

    def test_set_radii_clamps_to_minimum(self):
        self.ellipse.set_radii(0.0, -1.0)
        self.assertEqual(self.ellipse.rx, 0.001)
        self.assertEqual(self.ellipse.ry, 0.001)

    def test_set_angle(self):
        angle_rad = np.pi / 4.0
        self.ellipse.set_angle(angle_rad)
        self.assertAlmostEqual(self.ellipse.angle, angle_rad)

    def test_set_angle_none(self):
        self.ellipse.set_angle(None)
        self.assertEqual(self.ellipse.angle, 0.0)

    def test_evaluate_function_at_center(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(2.0, 1.0)
        result = self.ellipse.EvaluateFunction([0.0, 0.0])
        self.assertLess(result, 0.0)

    def test_evaluate_function_outside(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(1.0, 1.0)
        result = self.ellipse.EvaluateFunction([10.0, 10.0])
        self.assertGreater(result, 0.0)

    def test_evaluate_function_on_boundary(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(2.0, 1.0)
        result = self.ellipse.EvaluateFunction([2.0, 0.0])
        self.assertAlmostEqual(result, 0.0, places=5)

    def test_evaluate_function_with_rotation(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(2.0, 1.0)
        self.ellipse.set_angle(0.0)

        result_no_rot = self.ellipse.EvaluateFunction([2.0, 0.0])

        self.ellipse.set_angle(np.pi / 2.0)
        result_rotated = self.ellipse.EvaluateFunction([2.0, 0.0])

        self.assertNotAlmostEqual(result_no_rot, result_rotated, places=3)

    def test_evaluate_function_tuple_input(self):
        self.ellipse.set_center(0.0, 0.0)
        result = self.ellipse.EvaluateFunction((1.0, 0.0))
        self.assertIsInstance(result, (int, float, np.number))

    def test_evaluate_function_list_input(self):
        self.ellipse.set_center(0.0, 0.0)
        result = self.ellipse.EvaluateFunction([1.0, 0.0])
        self.assertIsInstance(result, (int, float, np.number))

    def test_evaluate_gradient_at_center(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(2.0, 1.0)
        gradient = [0.0, 0.0, 0.0]
        self.ellipse.EvaluateGradient([0.0, 0.0], gradient)
        self.assertAlmostEqual(gradient[0], 0.0, places=5)
        self.assertAlmostEqual(gradient[1], 0.0, places=5)
        self.assertAlmostEqual(gradient[2], 0.0, places=5)

    def test_evaluate_gradient_outside_ellipse(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(1.0, 1.0)
        gradient = [0.0, 0.0, 0.0]
        self.ellipse.EvaluateGradient([1.0, 1.0], gradient)
        self.assertGreater(gradient[0] ** 2 + gradient[1] ** 2, 0.0)
        self.assertAlmostEqual(gradient[2], 0.0, places=5)

    def test_evaluate_gradient_with_rotation(self):
        self.ellipse.set_center(0.0, 0.0)
        self.ellipse.set_radii(2.0, 1.0)

        self.ellipse.set_angle(0.0)
        grad1 = [0.0, 0.0, 0.0]
        self.ellipse.EvaluateGradient([1.0, 0.0], grad1)

        self.ellipse.set_angle(np.pi / 4.0)
        grad2 = [0.0, 0.0, 0.0]
        self.ellipse.EvaluateGradient([1.0, 0.0], grad2)

        self.assertNotAlmostEqual(grad1[0], grad2[0], places=3)

    def test_evaluate_gradient_zero_radii(self):
        self.ellipse.rx = 0.0
        self.ellipse.ry = 0.0
        gradient = [0.0, 0.0, 0.0]
        self.ellipse.EvaluateGradient([1.0, 1.0], gradient)
        self.assertEqual(gradient[0], 0.0)
        self.assertEqual(gradient[1], 0.0)
        self.assertEqual(gradient[2], 0.0)


class TestCylinderWidgetNoRotation(unittest.TestCase):
    def test_init(self):
        widget = CylinderWidgetNoRotation()
        self.assertIsNotNone(widget)

    def test_on_interaction_rotates_state_4_to_3(self):
        widget = CylinderWidgetNoRotation()

        mock_representation = mock.MagicMock()
        mock_representation.GetInteractionState.return_value = 4
        widget.GetCylinderRepresentation = mock.MagicMock(return_value=mock_representation)

        widget._on_interaction()

        mock_representation.SetInteractionState.assert_called_with(3)

    def test_on_interaction_preserves_other_states(self):
        widget = CylinderWidgetNoRotation()

        mock_representation = mock.MagicMock()
        mock_representation.GetInteractionState.return_value = 0
        widget.GetCylinderRepresentation = mock.MagicMock(return_value=mock_representation)

        widget._on_interaction()

        mock_representation.SetInteractionState.assert_not_called()


class TestRectangleWidgetNoRotation(unittest.TestCase):
    def test_init(self):
        widget = RectangleWidgetNoRotation()
        self.assertIsNotNone(widget)

    def test_on_interaction_rotates_state_8_to_7(self):
        widget = RectangleWidgetNoRotation()

        mock_representation = mock.MagicMock()
        mock_representation.GetInteractionState.return_value = 8
        widget.GetRepresentation = mock.MagicMock(return_value=mock_representation)

        widget._on_interaction()

        mock_representation.SetInteractionState.assert_called_with(7)

    def test_on_interaction_preserves_other_states(self):
        widget = RectangleWidgetNoRotation()

        mock_representation = mock.MagicMock()
        mock_representation.GetInteractionState.return_value = 0
        widget.GetRepresentation = mock.MagicMock(return_value=mock_representation)

        widget._on_interaction()

        mock_representation.SetInteractionState.assert_not_called()


class TestEllipseWidgetNoRotation(unittest.TestCase):
    def setUp(self):
        self.widget = EllipseWidgetNoRotation(plotter=None)

    def test_init(self):
        self.assertIsNotNone(self.widget._ellipse)
        self.assertIsNone(self.widget._plotter)
        self.assertEqual(self.widget._angle, 0.0)
        self.assertFalse(self.widget._rotating)
        self.assertIsNone(self.widget._center_display)
        self.assertIsNone(self.widget._handle_display)

    def test_set_ellipse_parameters(self):
        self.widget.set_ellipse_parameters(5.0, 3.0, 2.0, 1.5, angle=np.pi / 4.0)
        self.assertEqual(self.widget._ellipse.cx, 5.0)
        self.assertEqual(self.widget._ellipse.cy, 3.0)
        self.assertEqual(self.widget._ellipse.rx, 2.0)
        self.assertEqual(self.widget._ellipse.ry, 1.5)
        self.assertAlmostEqual(self.widget._angle, np.pi / 4)

    def test_set_ellipse_parameters_without_angle(self):
        self.widget._angle = np.pi / 2.0
        self.widget.set_ellipse_parameters(5.0, 3.0, 2.0, 1.5)
        self.assertEqual(self.widget._ellipse.cx, 5.0)
        self.assertAlmostEqual(self.widget._angle, np.pi / 2.0)

    def test_get_implicit_ellipse(self):
        self.widget._ellipse.set_center(1.0, 2.0)
        self.widget._ellipse.set_radii(2.0, 1.5)
        ellipse = self.widget.get_implicit_ellipse()
        self.assertIs(ellipse, self.widget._ellipse)

    def test_cancel_rotation_sets_flag(self):
        widget = EllipseWidgetNoRotation()
        widget._rotating = True
        widget._saved_interactor_style = None
        widget._cancel_rotation(None)
        self.assertFalse(widget._rotating)

    def test_disable_interactor_style(self):
        widget = EllipseWidgetNoRotation()
        mock_interactor = mock.MagicMock()
        mock_style = mock.MagicMock()
        mock_interactor.GetInteractorStyle.return_value = mock_style

        widget._disable_interactor_style(mock_interactor)

        self.assertEqual(widget._saved_interactor_style, mock_style)
        mock_interactor.SetInteractorStyle.assert_called_with(None)

    def test_disable_interactor_style_none_interactor(self):
        widget = EllipseWidgetNoRotation()
        widget._disable_interactor_style(None)

    def test_restore_interactor_style(self):
        widget = EllipseWidgetNoRotation()
        mock_interactor = mock.MagicMock()
        mock_style = mock.MagicMock()
        widget._saved_interactor_style = mock_style

        widget._restore_interactor_style(mock_interactor)

        mock_interactor.SetInteractorStyle.assert_called_with(mock_style)
        self.assertIsNone(widget._saved_interactor_style)

    def test_restore_interactor_style_none_saved(self):
        widget = EllipseWidgetNoRotation()
        mock_interactor = mock.MagicMock()
        widget._saved_interactor_style = None

        widget._restore_interactor_style(mock_interactor)

        mock_interactor.SetInteractorStyle.assert_not_called()

    def test_on_vtk_cancel_rotation_from_obj(self):
        widget = EllipseWidgetNoRotation()
        widget._rotating = True
        widget._saved_interactor_style = None

        mock_obj = mock.MagicMock()
        mock_obj.AbortFlagOn = mock.MagicMock()

        widget._on_vtk_cancel_rotation(mock_obj, None)

        self.assertFalse(widget._rotating)

    def test_on_vtk_end_interaction(self):
        widget = EllipseWidgetNoRotation()
        widget._rotating = True
        widget._saved_interactor_style = None

        mock_obj = mock.MagicMock()
        mock_obj.AbortFlagOn = mock.MagicMock()

        widget._on_vtk_end_interaction(mock_obj, None)

        self.assertFalse(widget._rotating)

    def test_on_interaction_state_8_to_7(self):
        widget = EllipseWidgetNoRotation()

        mock_representation = mock.MagicMock()
        mock_representation.GetInteractionState.return_value = 8
        widget.GetRepresentation = mock.MagicMock(return_value=mock_representation)

        widget._on_interaction()

        mock_representation.SetInteractionState.assert_called_with(7)

    def test_create_ellipse_mesh(self):
        widget = EllipseWidgetNoRotation()
        mesh = widget._create_ellipse_mesh(0.0, 0.0, 2.0, 1.0, 0.0, num_points=64, angle=0.0)

        self.assertIsNotNone(mesh)
        self.assertEqual(mesh.n_points, 64)

    def test_create_ellipse_mesh_with_rotation(self):
        widget = EllipseWidgetNoRotation()

        mesh1 = widget._create_ellipse_mesh(0.0, 0.0, 2.0, 1.0, 0.0, num_points=256, angle=0.0)
        points1 = mesh1.points.copy()
        points1.sort(axis=0)

        mesh2 = widget._create_ellipse_mesh(0.0, 0.0, 1.0, 2.0, 0.0, num_points=256, angle=-np.pi / 2.0)
        points2 = mesh2.points.copy()
        points2.sort(axis=0)
        np.testing.assert_allclose(points1, points2, rtol=1e-4, atol=0.1)

    def test_cleanup(self):
        widget = EllipseWidgetNoRotation()
        widget._vtk_interactor = mock.MagicMock()
        widget._ellipse_actor = mock.MagicMock()
        widget._rotation_handle_actor = mock.MagicMock()

        result = widget.cleanup()

        self.assertIs(result, widget._ellipse)


if __name__ == "__main__":
    unittest.main()
