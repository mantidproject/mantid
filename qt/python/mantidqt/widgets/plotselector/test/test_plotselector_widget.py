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

from mantidqt.widgets.plotselector.presenter import PlotSelectorPresenter
from mantidqt.widgets.plotselector.widget import PlotSelectorWidget

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


@requires_qapp
class PlotSelectorWidgetTest(unittest.TestCase):

    def setUp(self):
        self.presenter = mock.Mock(spec=PlotSelectorPresenter)
        self.widget = PlotSelectorWidget(self.presenter)

    def test_setting_plot_names_sets_names_in_list_view(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.widget.set_plot_list(plot_names)

        list_model = self.widget.list_view.model()
        for index in range(list_model.rowCount()):
            item = list_model.item(index)
            self.assertEqual(item.data(Qt.DisplayRole), plot_names[index])

    def test_setting_plot_names_to_empty_list(self):
        plot_names = []
        self.widget.set_plot_list(plot_names)

        list_model = self.widget.list_view.model()
        self.assertEqual(list_model.rowCount(), 0)

    def test_getting_all_selected_plot_names(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.widget.set_plot_list(plot_names)

        self.widget.list_view.selectAll()
        selected_plots = self.widget.get_all_selected_plot_names()
        self.assertEqual(selected_plots, plot_names)

    def test_getting_all_selected_plot_names_with_nothing_selected_returns_empty_list(self):
        selected_plots = self.widget.get_all_selected_plot_names()
        self.assertEqual(selected_plots, [])

    def test_getting_currently_selected_plot_name(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.widget.set_plot_list(plot_names)

        # It would be nice to avoid this, but could not find a way to
        # get the list item as a QWidget
        model_index = self.widget.list_view.model().index(1, 0)
        item_center = self.widget.list_view.visualRect(model_index).center()
        QTest.mouseClick(self.widget.list_view.viewport(), Qt.LeftButton, pos=item_center)

        selected_plot = self.widget.get_currently_selected_plot_name()
        self.assertEquals(selected_plot, plot_names[1])

    def test_getting_currently_selected_plot_name_with_nothing_selected_returns_None(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.widget.set_plot_list(plot_names)

        selected_plot = self.widget.get_currently_selected_plot_name()
        self.assertEquals(selected_plot, None)

    # ------------------------ Plot Closing -------------------------

    def test_close_button_pressed_calls_presenter(self):
        QTest.mouseClick(self.widget.close_button, Qt.LeftButton)
        self.assertEquals(self.presenter.close_action_called.call_count, 1)
        QTest.mouseClick(self.widget.close_button, Qt.LeftButton)
        self.assertEquals(self.presenter.close_action_called.call_count, 2)

    def test_delete_key_pressed_calls_presenter(self):
        QTest.keyClick(self.widget.close_button, Qt.Key_Delete)
        self.assertEquals(self.presenter.close_action_called.call_count, 1)
        QTest.keyClick(self.widget.close_button, Qt.Key_Delete)
        self.assertEquals(self.presenter.close_action_called.call_count, 2)

    # ----------------------- Plot Filtering ------------------------

    def test_filter_text_typing_calls_presenter_and_sets_filter_text(self):
        QTest.keyClicks(self.widget.filter_box, 'plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 5)
        self.assertEquals(self.widget.get_filter_text(), 'plot1')

    def test_programtic_filter_box_change_calls_presenter_and_sets_filter_text(self):
        self.widget.filter_box.setText('plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 1)
        self.assertEquals(self.widget.get_filter_text(), 'plot1')

    # ----------------------- Plot Selection ------------------------

    def test_plot_name_double_clicked_calls_presenter_and_makes_plot_current(self):
        plot_names = ["Plot1", "Plot2", "Plot3"]
        self.widget.set_plot_list(plot_names)

        model_index = self.widget.list_view.model().index(1, 0)
        item_center = self.widget.list_view.visualRect(model_index).center()
        # This single click should not be required, but otherwise the double click is not registered
        QTest.mouseClick(self.widget.list_view.viewport(), Qt.LeftButton, pos=item_center)
        QTest.mouseDClick(self.widget.list_view.viewport(), Qt.LeftButton, pos=item_center)

        self.assertEqual(self.presenter.list_double_clicked.call_count, 1)
        self.assertEqual(self.widget.get_currently_selected_plot_name(), plot_names[1])


if __name__ == '__main__':
    unittest.main()
