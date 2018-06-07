#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import, division, print_function

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from mantidqt.utils.qt.testing import requires_qapp

from workbench.widgets.plotselector.presenter import PlotSelectorPresenter
from workbench.widgets.plotselector.view import PlotSelectorView

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


@requires_qapp
class PlotSelectorWidgetTest(unittest.TestCase):

    def setUp(self):
        self.presenter = mock.Mock(spec=PlotSelectorPresenter)
        self.view = PlotSelectorView(self.presenter)

    def get_widget_by_row_number(self, row_number):
        item = self.view.list_widget.item(row_number)
        return self.view.list_widget.itemWidget(item)

    def test_setting_plot_names_sets_names_in_list_view(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        for index in range(len(self.view.list_widget)):
            widget = self.get_widget_by_row_number(index)
            self.assertEqual(widget.label.text(), plot_names[index])

    def test_setting_plot_names_to_empty_list(self):
        plot_names = []
        self.view.set_plot_list(plot_names)
        self.assertEqual(len(self.view.list_widget), 0)

    def test_getting_all_selected_plot_names(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        self.view.list_widget.selectAll()
        selected_plots = self.view.get_all_selected_plot_names()
        self.assertEqual(selected_plots, plot_names)

    def test_getting_all_selected_plot_names_with_nothing_selected_returns_empty_list(self):
        selected_plots = self.view.get_all_selected_plot_names()
        self.assertEqual(selected_plots, [])

    def test_getting_currently_selected_plot_name(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        # It would be nice to avoid this, but could not find a way to
        # get the list item as a QWidget
        model_index = self.view.list_widget.model().index(1, 0)
        item_center = self.view.list_widget.visualRect(model_index).center()
        QTest.mouseClick(self.view.list_widget.viewport(), Qt.LeftButton, pos=item_center)

        selected_plot = self.view.get_currently_selected_plot_name()
        self.assertEquals(selected_plot, plot_names[1])

    def test_getting_currently_selected_plot_name_with_nothing_selected_returns_None(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        selected_plot = self.view.get_currently_selected_plot_name()
        self.assertEquals(selected_plot, None)

    # ------------------------ Plot Closing -------------------------

    def test_close_button_pressed_calls_presenter(self):
        QTest.mouseClick(self.view.close_button, Qt.LeftButton)
        self.assertEquals(self.presenter.close_action_called.call_count, 1)
        QTest.mouseClick(self.view.close_button, Qt.LeftButton)
        self.assertEquals(self.presenter.close_action_called.call_count, 2)

    def test_delete_key_pressed_calls_presenter(self):
        QTest.keyClick(self.view.close_button, Qt.Key_Delete)
        self.assertEquals(self.presenter.close_action_called.call_count, 1)
        QTest.keyClick(self.view.close_button, Qt.Key_Delete)
        self.assertEquals(self.presenter.close_action_called.call_count, 2)

    def test_x_button_pressed_calls_presenter(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        widget = self.get_widget_by_row_number(1)
        QTest.mouseClick(widget.x_button, Qt.LeftButton)
        self.presenter.close_single_plot.assert_called_once_with("Plot2")

    def test_x_button_pressed_leaves_selection_unchanged(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        # Set the selected items by clicking with control held
        for row in [0, 2]:
            widget = self.get_widget_by_row_number(row)
            QTest.mouseClick(widget, Qt.LeftButton, Qt.ControlModifier)
        plots_selected_old = self.view.get_all_selected_plot_names()
        self.assertEquals(plots_selected_old, ["Plot1", "Plot3"])

        widget = self.get_widget_by_row_number(1)
        QTest.mouseClick(widget.x_button, Qt.LeftButton)

        # We need to actually update the plot list, as the presenter would
        self.view.remove_from_plot_list("Plot2")
        self.presenter.close_single_plot.assert_called_once_with("Plot2")

        plots_selected_new = self.view.get_all_selected_plot_names()
        self.assertEquals(plots_selected_old, plots_selected_new)

    # ----------------------- Plot Filtering ------------------------

    def test_filter_text_typing_calls_presenter_and_sets_filter_text(self):
        QTest.keyClicks(self.view.filter_box, 'plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 5)
        self.assertEquals(self.view.get_filter_text(), 'plot1')

    def test_programtic_filter_box_change_calls_presenter_and_sets_filter_text(self):
        self.view.filter_box.setText('plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 1)
        self.assertEquals(self.view.get_filter_text(), 'plot1')

    # ----------------------- Plot Selection ------------------------

    def test_plot_name_double_clicked_calls_presenter_and_makes_plot_current(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.view.set_plot_list(plot_names)

        model_index = self.view.list_widget.model().index(1, 0)
        item_center = self.view.list_widget.visualRect(model_index).center()
        # This single click should not be required, but otherwise the double click is not registered
        QTest.mouseClick(self.view.list_widget.viewport(), Qt.LeftButton, pos=item_center)
        QTest.mouseDClick(self.view.list_widget.viewport(), Qt.LeftButton, pos=item_center)

        self.assertEqual(self.presenter.list_double_clicked.call_count, 1)
        self.assertEqual(self.view.get_currently_selected_plot_name(), plot_names[1])


if __name__ == '__main__':
    unittest.main()
