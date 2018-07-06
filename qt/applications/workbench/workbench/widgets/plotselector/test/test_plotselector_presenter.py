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
from workbench.widgets.plotselector.view import PlotSelectorView, SortType

import os

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

    # ------------------------ Plot Updates ------------------------

    def test_plot_list_update(self):
        self.presenter.update_plot_list()
        self.assertEqual(self.model.update_plot_list.call_count, 1)
        self.view.set_plot_list.assert_called_once_with(["Plot1", "Plot2", "Plot3", "Graph99"])

    def test_append_to_plot_list_calls_update_in_model_and_view(self):
        self.presenter.append_to_plot_list("Plot4")
        self.model.append_to_plot_list.assert_called_once_with("Plot4")
        self.view.append_to_plot_list.assert_called_once_with("Plot4", True)

    def test_append_to_plot_list_with_error_on_model_does_not_update_view(self):
        self.model.append_to_plot_list.side_effect = ValueError("Some problem")
        self.presenter.append_to_plot_list("Plot1")
        self.model.append_to_plot_list.assert_called_once_with("Plot1")
        self.view.append_to_plot_list.assert_not_called()

    def test_remove_from_plot_list_calls_update_in_model_and_view(self):
        self.presenter.remove_from_plot_list("Plot1")
        self.model.remove_from_plot_list.assert_called_once_with("Plot1")
        self.view.remove_from_plot_list.assert_called_once_with("Plot1")

    def test_remove_from_plot_with_error_on_model_does_not_update_view(self):
        self.model.remove_from_plot_list.side_effect = ValueError("Some problem")
        self.presenter.remove_from_plot_list("Plot1")
        self.model.remove_from_plot_list.assert_called_once_with("Plot1")
        self.view.remove_from_plot_list.assert_not_called()

    def test_rename_in_plot_list_calls_update_in_model_and_view(self):
        self.presenter.rename_in_plot_list("Plot4", "Plot1")
        self.model.rename_in_plot_list.assert_called_once_with("Plot4", "Plot1")
        self.view.rename_in_plot_list.assert_called_once_with("Plot4", "Plot1")

    def test_rename_in_plot_list_with_error_from_model_does_no_update_view(self):
        self.model.rename_in_plot_list.side_effect = ValueError("Some problem")
        self.presenter.rename_in_plot_list("Plot4", "Plot1")
        self.model.rename_in_plot_list.assert_called_once_with("Plot4", "Plot1")
        self.view.rename_in_plot_list.assert_not_called()

    def test_rename_in_plot_list_with_same_old_and_new_name_does_nothing(self):
        self.presenter.rename_in_plot_list("Plot1", "Plot1")
        self.model.rename_in_plot_list.assert_not_called()
        self.view.rename_in_plot_list.assert_not_called()

    # ----------------------- Plot Filtering ------------------------

    def test_no_filtering_displays_all_plots(self):
        self.presenter.filter_text_changed()
        self.view.unhide_all_plots.assert_called_once_with()

    def test_plots_filtered_on_full_name(self):
        self.view.get_filter_text = mock.Mock(return_value="Plot1")
        self.presenter.filter_text_changed()

        self.view.filter_plot_list.assert_called_once_with(["Plot1"])

    def test_plots_filtered_on_substring(self):
        self.view.get_filter_text = mock.Mock(return_value="lot")
        self.presenter.filter_text_changed()

        self.view.filter_plot_list.assert_called_once_with(["Plot1", "Plot2", "Plot3"])

    def test_filtering_case_invariant(self):
        self.view.get_filter_text = mock.Mock(return_value="pLOT1")
        self.presenter.filter_text_changed()

        self.view.filter_plot_list.assert_called_once_with(["Plot1"])

    def test_append_to_plot_list_with_filter_set_matching_new_name(self):
        self.view.get_filter_text = mock.Mock(return_value="lot")

        self.presenter.append_to_plot_list("Plot4")
        self.model.append_to_plot_list.assert_called_once_with("Plot4")
        self.view.append_to_plot_list.assert_called_once_with("Plot4", True)

    def test_append_to_plot_list_with_filter_set_not_matching_new_name(self):
        self.view.get_filter_text = mock.Mock(return_value="Graph99")

        self.presenter.append_to_plot_list("Plot4")
        self.model.append_to_plot_list.assert_called_once_with("Plot4")
        self.view.append_to_plot_list.assert_called_once_with("Plot4", False)

    # ------------------------ Plot Showing ------------------------

    def test_show_single_plot_shows_it(self):
        self.view.get_currently_selected_plot_name = mock.Mock(return_value="Plot2")
        self.presenter.show_single_selected()
        self.model.show_plot.assert_called_once_with("Plot2")

    def test_show_multiple_plots_shows_them(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=["Plot2", "Plot3"])
        self.presenter.show_multiple_selected()
        self.assertEqual(self.model.show_plot.mock_calls[0], mock.call("Plot2"))
        self.assertEqual(self.model.show_plot.mock_calls[1], mock.call("Plot3"))

    # ------------------------ Plot Renaming ------------------------

    def test_rename_figure_calls_rename_in_model(self):
        self.presenter.rename_figure("NewName", "Plot1")
        self.model.rename_figure.assert_called_once_with("NewName", "Plot1")

    def test_rename_figure_with_same_old_and_new_name_does_nothing(self):
        self.presenter.rename_figure("Plot1", "Plot1")
        self.model.rename_figure.assert_not_called()

    def test_rename_figure_raising_a_value_error_undoes_rename_in_view(self):
        self.model.rename_figure.side_effect = ValueError("Some problem")
        self.presenter.rename_figure("NewName", "Plot1")
        self.model.rename_figure.assert_called_once_with("NewName", "Plot1")
        self.view.rename_in_plot_list.assert_called_once_with("Plot1", "Plot1")

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

    def test_close_action_with_model_call_raising_value_error(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=["Plot1"])
        self.model.close_plot.side_effect = ValueError("Some problem")

        self.presenter.close_action_called()
        self.model.close_plot.assert_called_once_with("Plot1")

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

    # ----------------------- Plot Sorting --------------------------

    def test_set_sort_order_to_ascending_calls_view_update(self):
        self.presenter.set_sort_order(is_ascending=True)
        self.view.sort_ascending.assert_called_once_with()

    def test_set_sort_order_to_descending_calls_view_update(self):
        self.presenter.set_sort_order(is_ascending=False)
        self.view.sort_descending.assert_called_once_with()

    def test_set_sort_type_to_name(self):
        self.view.sort_type = SortType.Name
        self.presenter.set_sort_type(is_by_name=True)
        self.view.sort_by_name.assert_called_once_with()
        self.view.set_sort_keys.assert_called_once_with({'Plot1': 'Plot1',
                                                         'Plot2': 'Plot2',
                                                         'Plot3': 'Plot3',
                                                         'Graph99': 'Graph99'})

    def test_set_sort_type_to_last_shown(self):
        self.model.last_active_order = mock.Mock(return_value={"Plot1": 1, "Plot2": 2})
        self.view.sort_type = SortType.LastShown
        self.presenter.set_sort_type(is_by_name=False)
        self.view.sort_by_last_shown.assert_called_once_with()
        self.view.set_sort_keys.assert_called_once_with({'Plot1': 1,
                                                         'Plot2': 2,
                                                         'Plot3': '_Plot3',
                                                         'Graph99': '_Graph99'})

    def test_update_last_shown_calls_model_and_view(self):
        self.model.last_active_order = mock.Mock(return_value={"Plot1": 1, "Plot2": 2})
        self.view.sort_type = SortType.LastShown
        self.presenter.update_sort_keys()

        self.model.last_active_order.assert_called_once_with()
        self.view.set_sort_keys.assert_called_once_with({'Plot1': 1,
                                                         'Plot2': 2,
                                                         'Plot3': '_Plot3',
                                                         'Graph99': '_Graph99'})

    def test_update_last_shown_with_sorting_by_name_does_nothing(self):
        self.model.last_active_order = mock.Mock(return_value={"Plot1": 1, "Plot2": 2})
        self.view.sort_type = SortType.Name
        self.presenter.update_sort_keys()

        self.model.last_active_order.assert_not_called()
        self.view.set_sort_keys.assert_not_called()

    def test_get_initial_sort_key_for_sort_by_name(self):
        self.view.sort_type = SortType.Name
        self.assertEqual(self.presenter.get_initial_sort_key("Plot1"), "Plot1")

    def test_get_initial_sort_key_for_last_shown_by_name(self):
        self.view.sort_type = SortType.LastShown
        self.assertEqual(self.presenter.get_initial_sort_key("Plot1"), "_Plot1")

    def test_get_renamed_sort_key_for_sort_by_name(self):
        self.view.sort_type = SortType.Name
        self.assertEqual(self.presenter.get_renamed_sort_key("Plot1", "Plot1"), "Plot1")

    def test_get_renamed_sort_key_for_sort_by_last_shown_with_int(self):
        self.view.sort_type = SortType.LastShown
        self.assertEqual(self.presenter.get_renamed_sort_key("Plot1", 1), 1)

    def test_get_renamed_sort_key_for_sort_by_last_shown_with_string(self):
        self.view.sort_type = SortType.LastShown
        self.assertEqual(self.presenter.get_renamed_sort_key("Plot1", "_Plot1"), "_Plot1")

    # ---------------------- Plot Exporting -------------------------

    def test_exporting_single_plot_generates_correct_filename(self):
        self.presenter.export_plot('Plot1', '/home/Documents', '.xyz')
        self.model.export_plot.assert_called_once_with('Plot1', '/home/Documents' + os.sep + 'Plot1.xyz')

    def test_exporting_multiple_plots_generates_correct_filename(self):
        self.view.get_all_selected_plot_names = mock.Mock(return_value=["Plot0", "Plot1", "Plot2"])
        self.presenter.export_plots('/home/Documents', '.xyz')
        for i in range(len(self.model.export_plot.mock_calls)):
            self.assertEqual(self.model.export_plot.mock_calls[i],
                             mock.call('Plot{}'.format(i), '/home/Documents' + os.sep + 'Plot{}.xyz'.format(i)))


if __name__ == '__main__':
    unittest.main()
