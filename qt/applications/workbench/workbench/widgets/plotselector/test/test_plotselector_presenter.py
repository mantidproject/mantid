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

from workbench.widgets.plotselector.model import PlotSelectorModel
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter
from workbench.widgets.plotselector.view import PlotSelectorView

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


class PlotSelectorPresenterTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=PlotSelectorView)
        self.view.get_filter_text = mock.Mock(return_value="")

        self.model = mock.Mock(spec=PlotSelectorModel)
        self.model.configure_mock(plot_list=["Plot1", "Plot2", "Plot3", "Graph99"])

        self.presenter = PlotSelectorPresenter(None, self.view, self.model)
        self.presenter.widget = self.view
        self.presenter.model = self.model

        # Ignore calls during the setup
        self.view.reset_mock()
        self.model.reset_mock()

    def convert_list_to_calls(self, list_to_convert):
        call_list = []
        for item in list_to_convert:
            call_list.append(mock.call(item))
        return call_list

    def test_plot_list_update(self):
        self.presenter.update_plot_list()
        self.assertEqual(self.model.update_plot_list.call_count, 1)
        self.view.set_plot_list.assert_called_once_with(["Plot1", "Plot2", "Plot3", "Graph99"])

    def test_plot_list_update_with_filter_set(self):
        self.view.get_filter_text = mock.Mock(return_value="Graph99")
        self.presenter.update_plot_list()
        self.assertEqual(self.model.update_plot_list.call_count, 1)
        self.view.set_plot_list.assert_called_once_with(["Graph99"])

    # ------------------------ Plot Closing -------------------------

    def test_close_action_single_plot(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=["Plot1"])

        self.presenter.close_action_called()
        self.model.close_plot.assert_called_once_with("Plot1")

    def test_close_action_multiple_plots(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=["Plot1", "Plot3"])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 2)
        self.model.close_plot.assert_has_calls(self.convert_list_to_calls(["Plot1", "Plot3"]),
                                               any_order=True)

    def test_close_action_with_no_plots_open(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=[])
        self.model.configure_mock(plot_list=[])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 0)

    def test_close_action_with_no_plots_selected(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=[])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 0)

    def test_close_single_plot_called(self):
        self.presenter.close_single_plot("Plot2")
        self.model.close_plot.assert_called_once_with("Plot2")

    # ----------------------- Plot Filtering ------------------------

    def test_no_filtering_displays_all_plots(self):
        self.presenter.filter_text_changed()
        self.view.set_plot_list.assert_called_once_with(["Plot1", "Plot2", "Plot3", "Graph99"])

    def test_plots_filtered_on_full_name(self):
        self.view.get_filter_text = mock.Mock(return_value="Plot1")
        self.presenter.filter_text_changed()

        self.view.set_plot_list.assert_called_once_with(["Plot1"])

    def test_plots_filtered_on_substring(self):
        self.view.get_filter_text = mock.Mock(return_value="lot")
        self.presenter.filter_text_changed()

        self.view.set_plot_list.assert_called_once_with(["Plot1", "Plot2", "Plot3"])

    def test_filtering_case_invariant(self):
        self.view.get_filter_text = mock.Mock(return_value="pLOT1")
        self.presenter.filter_text_changed()

        self.view.set_plot_list.assert_called_once_with(["Plot1"])

    # ----------------------- Plot Selection ------------------------

    def test_double_clicking_plot_brings_to_front(self):
        self.view.get_currently_selected_plot_name = mock.Mock(return_value="Plot2")
        self.presenter.list_double_clicked()

        self.model.make_plot_active.assert_called_once_with("Plot2")


if __name__ == '__main__':
    unittest.main()
