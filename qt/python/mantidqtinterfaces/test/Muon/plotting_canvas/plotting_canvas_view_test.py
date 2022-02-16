# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Muon.GUI.Common.contexts.plotting_context import PlottingContext
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view import PlottingCanvasView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import QuickEditWidget

import numpy as np


@start_qapplication
class PlottingCanvasViewTest(unittest.TestCase):

    def setUp(self):
        self.context = PlottingContext()
        self.settings = self.context.settings
        self.quick_edit = QuickEditWidget(self.context)
        self.view = PlottingCanvasView(self.quick_edit.widget, self.settings)
        self.view.fig = mock.MagicMock()
        self.view.fig.tight_layout = mock.Mock()
        self.view.show()
        self._count = -1

    def tearDown(self):
        self.assertTrue(self.view.close())

    def make_plot_side_effect(self, _unused):
        self._count +=1
        return self._count

    def test_start_up(self):
        self.assertEqual(self.view.fig.tight_layout.call_count, 1)

    def test_create_new_plot_canvas(self):
        num_axes = 5
        self.view.get_plot_fig = mock.Mock(return_value = (mock.Mock(), mock.Mock()))
        self.settings.set_condensed(False)
        self.view.fig.subplots_adjust = mock.Mock()

        self.view.create_new_plot_canvas(num_axes)
        self.assertEqual(self.view.fig.tight_layout.call_count, 2)
        self.assertEqual(self.view.fig.subplots_adjust.call_count, 0)

    def test_create_new_plot_canvas_condense(self):
        num_axes = 5
        self.view.get_plot_fig = mock.Mock(return_value = (mock.Mock(), mock.Mock()))
        self.settings.set_condensed(True)
        self.view.fig.subplots_adjust = mock.Mock()

        self.view.create_new_plot_canvas(num_axes)
        # 1 from start up
        self.assertEqual(self.view.fig.tight_layout.call_count, 1)
        self.assertEqual(self.view.fig.subplots_adjust.call_count, 1)

    def test_add_workspace_to_plot(self):
        self.view._make_plot = mock.Mock(side_effect = self.make_plot_side_effect)
        self.view._number_of_axes = 4
        self.settings.set_condensed(False)
        self.view.hide_axis = mock.Mock()

        ws_list = [mock.Mock() for j in range(self.view._number_of_axes)]

        self.view.add_workspaces_to_plot(ws_list)
        self.assertEqual(self.view.hide_axis.call_count, 0)

    def test_add_workspace_to_plot_condensed(self):
        self.view._make_plot = mock.Mock(side_effect = self.make_plot_side_effect)
        self.view._number_of_axes = 4
        self.settings.set_condensed(True)
        self.view.hide_axis = mock.Mock()

        ws_list = [mock.Mock() for j in range(self.view._number_of_axes)]

        self.view.add_workspaces_to_plot(ws_list)
        self.assertEqual(self.view.hide_axis.call_count, self.view._number_of_axes)
        for j in range(self.view._number_of_axes):
            self.view.hide_axis.assert_any_call(j, 2, 2)

    def test_add_workspace_to_plot_condensed_with_empty(self):
        self.view._make_plot = mock.Mock(side_effect = self.make_plot_side_effect)
        self.view._number_of_axes = 7 # 2 empty
        self.settings.set_condensed(True)
        self.view.hide_axis = mock.Mock()

        ws_list = [mock.Mock() for j in range(self.view._number_of_axes)]

        self.view.add_workspaces_to_plot(ws_list)
        self.assertEqual(self.view.hide_axis.call_count, 9)
        for j in range(self.view._number_of_axes):
            self.view.hide_axis.assert_any_call(j, 3, 3)
        # check empty axis
        self.view.hide_axis.assert_any_call(7, 3, 3)
        self.view.hide_axis.assert_any_call(8, 3, 3)

    def setup_axis_mock(self):
        ax = mock.Mock()
        ax.get_xticks = mock.Mock(return_value=np.array([0,1,2,3,4]))
        ax.set_xtixklabels = mock.Mock()
        ax.xaxis = mock.Mock()
        ax.xaxis.label = mock.Mock()
        ax.xaxis.label.set_visible = mock.Mock()

        ax.get_yticks = mock.Mock(return_value=np.array([0,1,2,3,4]))
        ax.set_ytixklabels = mock.Mock()
        ax.yaxis = mock.Mock()
        ax.yaxis.label = mock.Mock()
        ax.yaxis.label.set_visible = mock.Mock()

        ax.yaxis.set_label_position = mock.Mock()
        ax.yaxis.tick_right = mock.Mock()
        ax.set_title = mock.Mock()
        return ax

    def test_hide_axis_do_nothing(self):
        ax = self.setup_axis_mock()
        n_row = 2
        n_col = 2
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # bottom left
        self.view.hide_axis(2, n_row, n_col)

        self.assertEqual(0, ax.set_xticklabels.call_count)
        self.assertEqual(0, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.set_yticklabels.call_count)
        self.assertEqual(0, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.yaxis.set_label_position.call_count)
        self.assertEqual(0, ax.yaxis.tick_right.call_count)

    def test_hide_axis_x_only(self):
        ax = self.setup_axis_mock()
        n_row = 2
        n_col = 2
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # top left
        self.view.hide_axis(0, n_row, n_col)

        self.assertEqual(1, ax.set_xticklabels.call_count)
        self.assertEqual(1, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.set_yticklabels.call_count)
        self.assertEqual(0, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.yaxis.set_label_position.call_count)
        self.assertEqual(0, ax.yaxis.tick_right.call_count)

    def test_hide_axis_flip_y_only(self):
        ax = self.setup_axis_mock()
        n_row = 2
        n_col = 2
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # bottom right
        self.view.hide_axis(3, n_row, n_col)

        self.assertEqual(0, ax.set_xticklabels.call_count)
        self.assertEqual(0, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.set_yticklabels.call_count)
        self.assertEqual(0, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(1, ax.yaxis.set_label_position.call_count)
        self.assertEqual(1, ax.yaxis.tick_right.call_count)

    def test_hide_axis_flip_y_and_hide_x(self):
        ax = self.setup_axis_mock()
        n_row = 2
        n_col = 2
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # top right
        self.view.hide_axis(1, n_row, n_col)

        self.assertEqual(1, ax.set_xticklabels.call_count)
        self.assertEqual(1, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.set_yticklabels.call_count)
        self.assertEqual(0, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(1, ax.yaxis.set_label_position.call_count)
        self.assertEqual(1, ax.yaxis.tick_right.call_count)

    def test_hide_axis_hide_y(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # last row, middle tile
        self.view.hide_axis(7, n_row, n_col)

        self.assertEqual(0, ax.set_xticklabels.call_count)
        self.assertEqual(0, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(1, ax.set_yticklabels.call_count)
        self.assertEqual(1, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.yaxis.set_label_position.call_count)
        self.assertEqual(0, ax.yaxis.tick_right.call_count)

    def test_hide_axis_hide_y_and_x(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # middle row and middle col
        self.view.hide_axis(4, n_row, n_col)

        self.assertEqual(1, ax.set_xticklabels.call_count)
        self.assertEqual(1, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(1, ax.set_yticklabels.call_count)
        self.assertEqual(1, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.yaxis.set_label_position.call_count)
        self.assertEqual(0, ax.yaxis.tick_right.call_count)

    def test_hide_axis_flip_y_and_hide_x_9(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        axes = [ax for j in range(n_row*n_col)]

        self.view.fig.axes = axes
        # middle row and right subplot
        self.view.hide_axis(5, n_row, n_col)

        self.assertEqual(1, ax.set_xticklabels.call_count)
        self.assertEqual(1, ax.xaxis.label.set_visible.call_count)
        self.assertEqual(0, ax.set_yticklabels.call_count)
        self.assertEqual(0, ax.yaxis.label.set_visible.call_count)
        self.assertEqual(1, ax.yaxis.set_label_position.call_count)
        self.assertEqual(1, ax.yaxis.tick_right.call_count)

    def test_set_title(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        self.view._number_of_axes = n_row*n_col
        axes = [ax for j in range(n_row*n_col)]
        self.view.fig.axes = axes

        self.view.set_title(0, "test")
        self.assertEqual(1, ax.set_title.call_count)
        ax.set_title.assert_called_once_with("test")

    def test_set_title_out_of_bounds(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        self.view._number_of_axes = n_row*n_col
        axes = [ax for j in range(n_row*n_col)]
        self.view.fig.axes = axes

        self.view.set_title(100, "test")
        self.assertEqual(0, ax.set_title.call_count)

    def test_set_title_condense(self):
        ax = self.setup_axis_mock()
        n_row = 3
        n_col = 3
        self.view._number_of_axes = n_row*n_col
        axes = [ax for j in range(n_row*n_col)]
        self.view.fig.axes = axes
        self.settings.set_condensed(True)
        self.view.set_title(1, "test")
        self.assertEqual(0, ax.set_title.call_count)

    def test_redraw_figure(self):
        self.view._redraw_legend = mock.Mock()
        self.settings.set_condensed(False)
        self.view.redraw_figure()
        # one from start up
        self.assertEqual(2, self.view.fig.tight_layout.call_count)

    def test_redraw_figure_condense(self):
        self.view._redraw_legend = mock.Mock()
        self.settings.set_condensed(True)
        self.view.redraw_figure()
        # one from start up
        self.assertEqual(1, self.view.fig.tight_layout.call_count)

    def test_resizeEvent(self):
        self.settings.set_condensed(False)
        self.view.resizeEvent(mock.Mock())
        # one from start up
        self.assertEqual(2, self.view.fig.tight_layout.call_count)

    def test_resizeEvent_condense(self):
        self.settings.set_condensed(True)
        self.view.resizeEvent(mock.Mock())
        # one from start up
        self.assertEqual(1, self.view.fig.tight_layout.call_count)

    def mock_axis(self):
        axes = mock.Mock()
        axes.cla = mock.Mock()
        axes.tracked_workspaces = mock.Mock()
        axes.tracked_workspaces.clear = mock.Mock()
        axes.set_prop_cycle = mock.Mock()

        return axes

    def test_clear_all_workspaces_from_plot(self):
        self.view.fig = mock.Mock()
        axis = self.mock_axis()
        self.view.fig.axes = [axis]

        color_queue = mock.Mock()
        color_queue.reset = mock.Mock()
        self.view._color_queue = [color_queue]

        shade_region = mock.Mock()
        shade_region.remove = mock.Mock()
        self.view._shaded_regions = [shade_region]

        self.view.clear_all_workspaces_from_plot()

        axis.cla.assert_called_once_with()
        axis.tracked_workspaces.clear.assert_called_once_with()
        axis.set_prop_cycle.assert_called_once_with(None)

        color_queue.reset.assert_called_once_with()

        shade_region.remove.assert_called_once_with()

    def test_add_shaded_region(self):
        self.view.fig.axes = [1,2]
        x_data = [0,1,2,3]
        y1_data = [1,2,3,4]
        y2_data = [ 4,3,2,1]
        axis = 1
        name = "unit test"

        self.assertEqual(self.view._shaded_regions, {})

        self.view.add_shaded_region(name, axis, x_data, y1_data, y2_data)

        self.assertEqual(list(self.view._shaded_regions.keys()), [name])
        self.assertEqual(self.view._shaded_regions[name].axis, 2)
        self.assertEqual(self.view._shaded_regions[name].x_values, x_data)
        self.assertEqual(self.view._shaded_regions[name].y1_values, y1_data)
        self.assertEqual(self.view._shaded_regions[name].y2_values, y2_data)

    def test_update_shaded_region(self):
        self.view.fig.axes = [1,2]
        x_data = [0,1,2,3]
        y1_data = [1,2,3,4]
        y2_data = [ 4,3,2,1]
        axis = 1
        name = "unit test"

        self.assertEqual(self.view._shaded_regions, {})

        self.view.add_shaded_region(name, axis, x_data, y1_data, y2_data)

        new_x = [5,4,3,2,1]
        new_y1 = [4,2,0,-2,-4]
        new_y2 = [1,2,3,4,5]

        self.view.add_shaded_region(name, axis, new_x, new_y1, new_y2)
        self.assertEqual(list(self.view._shaded_regions.keys()), [name])
        self.assertEqual(self.view._shaded_regions[name].axis, 2)
        self.assertEqual(self.view._shaded_regions[name].x_values, new_x)
        self.assertEqual(self.view._shaded_regions[name].y1_values, new_y1)
        self.assertEqual(self.view._shaded_regions[name].y2_values, new_y2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
