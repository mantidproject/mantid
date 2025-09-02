# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

from mantidqt.utils.qt.testing import start_qapplication
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from mantid.simpleapi import CreateSampleWorkspace


@start_qapplication
class TestFullInstrumentViewWindow(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._workspace = CreateSampleWorkspace(StoreInADS=False)

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    def setUp(self, mock_plotter, mock_v_add_widget, mock_h_add_widget, mock_figure_canvas) -> None:
        self._view = FullInstrumentViewWindow()
        self._mock_plotter = mock_plotter
        self._mock_v_add_widget = mock_v_add_widget
        self._mock_h_add_widget = mock_h_add_widget
        self._mock_figure_canvas = mock_figure_canvas
        self._view._presenter = MagicMock()

    def test_plotters_created(self):
        self.assertEqual(self._mock_plotter.call_count, 2)

    def test_figure_canvas_created(self):
        self._mock_figure_canvas.assert_called_once()

    def test_update_scalar_range(self):
        self._view.set_plotter_scalar_bar_range([0, 100], "label")
        self.assertEqual(2, self._view.main_plotter.update_scalar_bar_range.call_count)
        self._view.main_plotter.update_scalar_bar_range.assert_has_calls([mock.call([0, 100], "label"), mock.call([0, 100], "label")])

    @mock.patch("qtpy.QtWidgets.QMainWindow.closeEvent")
    def test_close_event(self, mock_close_event):
        self._view.closeEvent(MagicMock())
        self.assertEqual(2, mock_close_event.call_count)
        self.assertEqual(2, self._view.main_plotter.close.call_count)

    def test_add_simple_shape(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_colour = MagicMock()
        self._view.add_simple_shape(mock_mesh, mock_colour, False)
        self._view.main_plotter.add_mesh.assert_called_once_with(mock_mesh, color=mock_colour, pickable=False)

    def test_add_main_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        clim = [0, 250]
        self._view.add_main_mesh(mock_mesh, mock_scalars, clim)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh, pickable=False, scalars=mock_scalars, clim=clim, render_points_as_spheres=True, point_size=14
        )

    def test_add_pickable_main_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_pickable_main_mesh(mock_mesh, mock_scalars)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh,
            scalars=mock_scalars,
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def test_add_rgba_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_rgba_mesh(mock_mesh, mock_scalars)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh, scalars=mock_scalars, rgba=True, pickable=False, render_points_as_spheres=True, point_size=10
        )

    def test_enable_point_picking(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = False
        mock_callback = MagicMock()
        self._view.enable_point_picking(mock_callback)
        self._view.main_plotter.disable_picking.assert_has_calls([mock.call(), mock.call()])
        self._view.main_plotter.enable_surface_point_picking.assert_called_once_with(
            show_message=False,
            use_picker=True,
            callback=mock_callback,
            show_point=False,
            pickable_window=False,
            picker="point",
            tolerance=0.01,
        )
        self._view.projection_plotter.enable_point_picking.assert_called_once_with(
            show_message=False,
            use_picker=True,
            callback=mock_callback,
            show_point=False,
            pickable_window=False,
            picker="point",
            tolerance=0.01,
        )

    def test_enable_point_picking_off_screen(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = True
        mock_callback = MagicMock()
        self._view.enable_point_picking(mock_callback)
        self._view.main_plotter.disable_picking.assert_has_calls([mock.call(), mock.call()])
        self._view.main_plotter.enable_surface_point_picking.assert_not_called()
        self._view.projection_plotter.enable_point_picking.assert_not_called()

    def test_enable_rectangle_picking(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = False
        mock_callback = MagicMock()
        self._view.enable_rectangle_picking(mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.main_plotter.enable_rectangle_picking.assert_called_once_with(callback=mock_callback, use_picker=True, font_size=12)

    def test_enable_rectangle_picking_off_screen(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = True
        mock_callback = MagicMock()
        self._view.enable_rectangle_picking(mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.main_plotter.enable_rectangle_picking.assert_not_called()

    def test_add_projection_mesh(self):
        self._view.projection_plotter.reset_mock()
        self._view.projection_plotter.off_screen = False
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        clim = [100, 500]
        self._view.add_projection_mesh(mock_mesh, mock_scalars, clim)
        self._view.projection_plotter.clear.assert_called_once()
        self._view.projection_plotter.add_mesh.assert_called_once_with(
            mock_mesh, scalars=mock_scalars, clim=clim, render_points_as_spheres=True, point_size=14, pickable=False
        )
        self._view.projection_plotter.enable_zoom_style.assert_called_once()

    def test_add_projection_mesh_off_screen(self):
        self._view.projection_plotter.reset_mock()
        self._view.projection_plotter.off_screen = True
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        clim = [100, 500]
        self._view.add_projection_mesh(mock_mesh, mock_scalars, clim)
        self._view.projection_plotter.clear.assert_called_once()
        self._view.projection_plotter.add_mesh.assert_called_once_with(
            mock_mesh, scalars=mock_scalars, clim=clim, render_points_as_spheres=True, point_size=14, pickable=False
        )
        self._view.projection_plotter.enable_zoom_style.assert_not_called()

    def test_add_pickable_projection_mesh(self):
        self._view.projection_plotter.reset_mock()
        self._view.projection_plotter.off_screen = False
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_pickable_projection_mesh(mock_mesh, mock_scalars)
        self._view.projection_plotter.add_mesh.assert_called_once_with(
            mock_mesh,
            scalars=mock_scalars,
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def test_add_pickable_projection_mesh_off_screen(self):
        self._view.projection_plotter.reset_mock()
        self._view.projection_plotter.off_screen = True
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_pickable_projection_mesh(mock_mesh, mock_scalars)
        self._view.projection_plotter.add_mesh.assert_called_once_with(
            mock_mesh,
            scalars=mock_scalars,
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )
        self._view.projection_plotter.enable_image_style.assert_not_called()
