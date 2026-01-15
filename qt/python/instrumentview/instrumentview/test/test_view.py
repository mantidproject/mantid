# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

from vtkmodules.vtkInteractionWidgets import vtkBoxRepresentation
import numpy as np

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
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    def setUp(self, mock_plotter, mock_splitter_add_widget, mock_v_add_widget, mock_h_add_widget, mock_figure_canvas) -> None:
        self._view = FullInstrumentViewWindow()
        self._mock_plotter = mock_plotter
        self._mock_splitter_add_widget = mock_splitter_add_widget
        self._mock_v_add_widget = mock_v_add_widget
        self._mock_h_add_widget = mock_h_add_widget
        self._mock_figure_canvas = mock_figure_canvas
        self._view._presenter = MagicMock()

    def test_plotters_created(self):
        self._mock_plotter.assert_called_once()
        # self.assertEqual(self._mock_plotter.call_count, 2)

    def test_figure_canvas_created(self):
        self._mock_figure_canvas.assert_called_once()

    def test_update_scalar_range(self):
        self._view.set_plotter_scalar_bar_range((0, 100), "label")
        self._view.main_plotter.update_scalar_bar_range.assert_has_calls([mock.call((0, 100), "label")])

    @mock.patch("qtpy.QtWidgets.QMainWindow.closeEvent")
    def test_close_event(self, mock_close_event):
        self._view.closeEvent(MagicMock())
        self.assertEqual(2, mock_close_event.call_count)
        self._view.main_plotter.close.assert_called_once()

    def test_add_simple_shape(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_colour = MagicMock()
        self._view.add_simple_shape(mock_mesh, mock_colour, False)
        self._view.main_plotter.add_mesh.assert_called_once_with(mock_mesh, color=mock_colour, pickable=False)

    def test_add_detector_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_detector_mesh(mock_mesh, False, mock_scalars)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh,
            pickable=False,
            scalars=mock_scalars,
            render_points_as_spheres=True,
            point_size=15,
            scalar_bar_args={"interactive": True, "vertical": False, "title_font_size": 15, "label_font_size": 12},
        )

    def test_add_pickable_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_pickable_mesh(mock_mesh, mock_scalars)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh,
            scalars=mock_scalars,
            opacity=[0.0, 0.3],
            clim=[0, 1],
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
        self._view.interactor_style = MagicMock()
        self._view.enable_point_picking(False, mock_callback)
        self._view.interactor_style.remove_interactor.assert_called_once()
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.main_plotter.enable_surface_point_picking.assert_called_once_with(
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
        self._view.interactor_style = MagicMock()
        self._view.enable_point_picking(False, mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.interactor_style.remove_interactor.assert_called_once()
        self._view.main_plotter.enable_surface_point_picking.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewWindow.CustomInteractorStyleRubberBand3D")
    def test_enable_rectangle_picking_3d(self, mock_style_3d):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = False
        mock_callback = MagicMock()
        self._view.enable_rectangle_picking(False, mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        mock_style_3d.assert_called_once()

    def test_enable_rectangle_picking_2d(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = False
        self._view.interactor_style = MagicMock()
        mock_callback = MagicMock()
        self._view.enable_rectangle_picking(True, mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.interactor_style.set_interactor.assert_called_once()

    def test_enable_rectangle_picking_off_screen(self):
        self._view.main_plotter.reset_mock()
        self._view.main_plotter.off_screen = True
        mock_callback = MagicMock()
        self._view.enable_rectangle_picking(False, mock_callback)
        self._view.main_plotter.disable_picking.assert_called_once()
        self._view.main_plotter.enable_rectangle_picking.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewWindow.QListWidgetItem")
    def test_refresh_peaks_ws_list(self, mock_qlist_widget_item):
        mock_list = MagicMock()
        mock_item = MagicMock()
        mock_list.item.return_value = mock_item
        mock_list.count.return_value = 1
        mock_item.checkState.return_value = 0
        self._view._peak_ws_list = mock_list
        self._view._presenter.peaks_workspaces_in_ads.return_value = [MagicMock()]
        self._view.refresh_peaks_ws_list()
        mock_list.clear.assert_called_once()
        mock_list.adjustSize.assert_called_once()
        mock_qlist_widget_item.assert_called_once()

    def test_clear_overlay_meshes(self):
        mock_meshes = (MagicMock(), MagicMock())
        self._view._overlay_meshes = [mock_meshes]
        self._view.clear_overlay_meshes()
        self._view.main_plotter.remove_actor.assert_has_calls([mock.call(mock_meshes[0]), mock.call(mock_meshes[1])])
        self.assertEqual(0, len(self._view._overlay_meshes))

    def test_clear_lineplot_overlays(self):
        mock_line = MagicMock()
        mock_text = MagicMock()
        self._view._detector_spectrum_axes = MagicMock()
        self._view._detector_spectrum_axes.texts = [mock_text]
        self.assertEqual(0, len(self._view._lineplot_overlays))
        self._view._lineplot_overlays.append(mock_line)
        self._view.clear_lineplot_overlays()
        mock_line.remove.assert_called_once()
        self.assertEqual(0, len(self._view._lineplot_overlays))
        mock_text.remove.assert_called_once()

    def test_plot_overlay_mesh(self):
        position_groups = [[0, 0, 0]]
        self._view.plot_overlay_mesh(position_groups, MagicMock(), "colour")
        self._view.main_plotter.add_points.assert_called_once()
        self._view.main_plotter.add_point_labels.assert_called_once()
        self.assertEqual(1, len(self._view._overlay_meshes))

    def test_plot_lineplot_overlay(self):
        x_values = [1.0, 2.0]
        labels = ["a", "b"]
        self._view._detector_spectrum_axes = MagicMock()
        self._view.plot_lineplot_overlay(x_values, labels, "colour")
        self.assertEqual(2, self._view._detector_spectrum_axes.text.call_count)
        self.assertEqual(2, len(self._view._lineplot_overlays))

    def test_redraw_lineplot(self):
        self._view.redraw_lineplot()
        self._view._detector_figure_canvas.draw.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewWindow.vtkBoxRepresentation")
    @mock.patch("instrumentview.FullInstrumentViewWindow.RectangleWidgetNoRotation")
    def test_add_rectangular_widget(self, mock_widget, mock_repr) -> None:
        test_repr = vtkBoxRepresentation()
        mock_repr.return_value = test_repr
        self._view.main_plotter.renderer = MagicMock(GetSize=MagicMock(return_value=(1, 1)))
        self._view.display_to_world_coords = MagicMock(side_effect=[(-1, -2, 3), (1, 2, 3)])
        self._view.add_rectangular_widget()
        self._view.display_to_world_coords.assert_called_with(2 / 3, 2 / 3, 0)
        np.testing.assert_almost_equal(test_repr.bounds, [-1, 1, -2, 2, -0.1, 1])
        self.assertEqual(self._view._current_widget, mock_widget())

    @mock.patch("instrumentview.FullInstrumentViewWindow.vtkImplicitCylinderRepresentation")
    @mock.patch("instrumentview.FullInstrumentViewWindow.CylinderWidgetNoRotation")
    def test_add_cylinder_widget(self, mock_cylinder_widget, mock_repr_call) -> None:
        mock_repr = MagicMock()
        mock_repr_call.return_value = mock_repr
        self._view.main_plotter.renderer = MagicMock(GetSize=MagicMock(return_value=(1, 1)))
        self._view.display_to_world_coords = MagicMock(side_effect=[(-1, -2, 3), (1, 2, 3)])
        self._view.main_plotter.bounds = [1, 2, 1, 2, 1, 2]
        self._view.add_cylinder_widget()
        mock_repr.SetCenter.assert_called_with([-1, -2, 0.5])
        mock_repr.SetRadius.assert_called_with(np.sqrt(2**2 + 4**2))
        border = np.sqrt(1**2 + 1**2) / 2
        mock_repr.SetWidgetBounds.assert_called_with([1 - border, 2 + border, 1 - border, 2 + border, 0, 1])
        self.assertEqual(self._view._current_widget, mock_cylinder_widget())


if __name__ == "__main__":
    unittest.main()
