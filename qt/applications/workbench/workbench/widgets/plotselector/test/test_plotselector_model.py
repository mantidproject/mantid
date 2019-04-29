# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import, division, print_function

import unittest

from mantid.py3compat import mock
from workbench.plotting.globalfiguremanager import FigureAction
from workbench.widgets.plotselector.model import PlotSelectorModel
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter


class PlotSelectorModelTest(unittest.TestCase):

    def side_effect_manager(self, plot_number):
        if plot_number == 42:
            return self.figure_manager
        return None

    def setUp(self):
        self.presenter = mock.Mock(spec=PlotSelectorPresenter)

        self.figure_manager = mock.Mock()
        self.figure_manager.show = mock.Mock()
        self.figure_manager.get_window_title = mock.Mock(return_value="Plot1")
        self.figure_manager.is_visible = mock.Mock(return_value=True)

        self.global_figure_manager = mock.Mock()
        self.global_figure_manager.add_observer = mock.Mock()
        self.global_figure_manager.figs.get = mock.Mock(side_effect=self.side_effect_manager)
        self.global_figure_manager.destroy = mock.Mock()

        self.model = PlotSelectorModel(self.presenter, self.global_figure_manager)
        self.model.plot_list = ["Plot1", "Plot2"]

    # ------------------------ Plot Updates ------------------------

    def test_observer_added_during_setup(self):
        self.assertEqual(self.global_figure_manager.add_observer.call_count, 1)

    def test_notify_for_new_plot_calls_append_in_presenter(self):
        self.model.notify(FigureAction.New, 42)
        self.presenter.append_to_plot_list.assert_called_once_with(42)

    def test_notify_for_closed_plot_calls_removed_in_presenter(self):
        self.model.notify(FigureAction.Closed, 42)
        self.presenter.remove_from_plot_list.assert_called_once_with(42)

    def test_notify_for_renamed_plot_calls_rename_in_presenter(self):
        self.model.notify(FigureAction.Renamed, 42)
        self.presenter.rename_in_plot_list.assert_called_once_with(42, "Plot1")

    def test_notify_for_renamed_plot_with_invalid_figure_number_does_nothing(self):
        self.model.notify(FigureAction.Renamed, 0)
        self.presenter.rename_in_plot_list.assert_not_called()

    def test_notify_for_order_changed_calls_presenter(self):
        self.model.notify(FigureAction.OrderChanged, 42)
        self.presenter.update_last_active_order.assert_called_once_with()
        self.presenter.set_active_font.assert_called_once_with(42)

    def test_notify_for_order_changed_with_invalid_plot_number(self):
        self.model.notify(FigureAction.OrderChanged, -1)
        self.presenter.update_last_active_order.assert_called_once_with()
        self.presenter.set_active_font.assert_not_called()

    def test_notify_visibility_changed_calls_presesnter(self):
        self.model.notify(FigureAction.VisibilityChanged, 42)
        self.presenter.update_visibility_icon.assert_called_once_with(42)

    def test_notify_unknwon_updates_plot_list_in_presenter(self):
        self.model.notify(FigureAction.Update, -1)
        self.presenter.update_plot_list.assert_called_once_with()

    # ------------------------ Plot Showing ------------------------

    def test_show_plot_calls_current_figure(self):
        self.model.show_plot(42)
        self.assertEqual(self.figure_manager.show.call_count, 1)

    def test_show_plot_for_invalid_name_raises_value_error(self):
        self.assertRaises(ValueError, self.model.show_plot, 0)
        self.figure_manager.show.assert_not_called()

    # ------------------------ Plot Hiding -------------------------

    def test_is_visible_returns_true_for_visible_window(self):
        self.assertTrue(self.model.is_visible(42))

    def test_is_visible_for_invalid_number_raises_value_error(self):
        self.assertRaises(ValueError, self.model.is_visible, 0)

    def test_hide_plot_calls_hide_on_the_plot_window(self):
        self.model.hide_plot(42)
        self.figure_manager.window.hide.assert_called_once_with()

    def test_hide_plot_for_invalid_name_raises_value_error(self):
        self.assertRaises(ValueError, self.model.hide_plot, 0)
        self.figure_manager.window.hide.asser_not_called()

    # ------------------------ Plot Renaming ------------------------

    def test_renaming_calls_set_window_title(self):
        self.model.rename_figure(42, "NewName")
        self.figure_manager.set_window_title.assert_called_once_with("NewName")

    def test_renaming_calls_with_invalid_number_raises_value_error(self):
        self.assertRaises(ValueError, self.model.rename_figure, 0, "NewName")
        self.figure_manager.set_window_title.assert_not_called()

    # ------------------------ Plot Closing -------------------------

    def test_notify_for_closing_plot_calls_remove_in_presenter(self):
        self.model.notify(FigureAction.Closed, "Plot1")
        self.presenter.remove_from_plot_list.assert_called_once_with("Plot1")

    def test_close_plot_for_invalid_number_raises_value_error(self):
        self.assertRaises(ValueError, self.model.close_plot, 0)
        self.global_figure_manager.destroy.assert_not_called()

    def test_close_plot_calls_destroy_in_global_figure_manager(self):
        self.model.close_plot(42)
        self.global_figure_manager.destroy.assert_called_once_with(42)

    # ----------------------- Plot Sorting --------------------------

    def test_last_active_values_calls_global_figure_manager(self):
        self.model.last_active_values()
        self.global_figure_manager.last_active_values.assert_called_once_with()

    # ---------------------- Plot Exporting -------------------------

    def test_export_plot_calls_savefig_on_figure(self):
        self.model.export_plot(42, "/home/Documents/Figure1.pdf")
        self.figure_manager.canvas.figure.savefig.assert_called_once_with("/home/Documents/Figure1.pdf")


if __name__ == '__main__':
    unittest.main()
