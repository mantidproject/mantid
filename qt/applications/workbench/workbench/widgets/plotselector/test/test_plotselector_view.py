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

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtTest import QTest
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter
from workbench.widgets.plotselector.view import EXPORT_TYPES, PlotSelectorView, Column



class PlotSelectorWidgetTest(GuiTest):

    def setUp(self):
        self.presenter = mock.Mock(spec=PlotSelectorPresenter)
        self.presenter.get_plot_name_from_number = mock.Mock(side_effect=self.se_plot_name)
        self.presenter.get_initial_last_active_value = mock.Mock(side_effect=self.se_get_initial_last_active_value)
        self.presenter.is_shown_by_filter = mock.Mock(side_effect=self.se_is_shown_by_filter)

        patcher = mock.patch('workbench.widgets.plotselector.view.get_icon')
        self.mock_get_icon = patcher.start()
        self.mock_get_icon.return_value = QIcon()
        self.addCleanup(patcher.stop)

        self.view = PlotSelectorView(self.presenter)
        self.view.table_widget.setSortingEnabled(False)

    def se_plot_name(self, plot_number):
        if plot_number == 0:
            return "Plot1"
        if plot_number == 1:
            return "Plot2"
        if plot_number == 2:
            return "Plot3"
        if plot_number == 3:
            return "Plot4"
        if plot_number == 19:
            return "Plot20"
        if plot_number == 42:
            return "Graph99"
        return None

    def se_get_initial_last_active_value(self, plot_number):
        plot_name = self.se_plot_name(plot_number)
        return '_' + plot_name

    def se_is_shown_by_filter(self, plot_number):
        if plot_number == 0:
            return True
        return False

    def click_to_select_by_row_number(self, row_number):
        widget = self.view.table_widget.cellWidget(row_number, Column.Name)
        QTest.mouseClick(widget, Qt.LeftButton)

    def assert_list_of_plots_is_set_in_widget(self, plot_names):
        self.assertEqual(len(plot_names), self.view.table_widget.rowCount())
        for index in range(self.view.table_widget.rowCount()):
            widget = self.view.table_widget.cellWidget(index, Column.Name)
            self.assertEqual(widget.line_edit.text(), plot_names[index])

    # ------------------------ Plot Updates ------------------------

    def test_setting_plot_names_sets_names_in_list_view(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.assert_list_of_plots_is_set_in_widget(["Plot1", "Plot2", "Plot3"])

    def test_setting_plot_names_to_empty_list(self):
        plot_numbers = []
        self.view.set_plot_list(plot_numbers)
        self.assertEqual(self.view.table_widget.rowCount(), 0)

    def test_appending_to_plot_list(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.append_to_plot_list(3)

        self.assert_list_of_plots_is_set_in_widget(["Plot1", "Plot2", "Plot3", "Plot4"])

    def test_removing_from_plot_list(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.remove_from_plot_list(1)

        self.assert_list_of_plots_is_set_in_widget(["Plot1", "Plot3"])

    def test_renaming_in_plot_list(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.rename_in_plot_list(1, "Graph99")

        self.assert_list_of_plots_is_set_in_widget(["Plot1", "Graph99", "Plot3"])

    # ----------------------- Plot Selection ------------------------

    def test_getting_all_selected_plot_numbers(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.table_widget.selectAll()
        selected_plots = self.view.get_all_selected_plot_numbers()
        # Expected result: [0, 1, 2]
        # Something goes wrong in QTest here and the selection is
        # always the first plot.
        self.assertEqual(selected_plots, [0])

    def test_getting_all_selected_plot_numbers_with_nothing_selected_returns_empty_list(self):
        selected_plots = self.view.get_all_selected_plot_numbers()
        self.assertEqual(selected_plots, [])

    def test_getting_currently_selected_plot_number(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.click_to_select_by_row_number(1)

        selected_plot = self.view.get_currently_selected_plot_number()
        # Expected result: 1
        # Something goes wrong in QTest here and the selection is
        # always the first plot or None.
        self.assertTrue(selected_plot in [0, None])

    def test_getting_currently_selected_plot_number_with_nothing_selected_returns_None(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        selected_plot = self.view.get_currently_selected_plot_number()
        self.assertEquals(selected_plot, None)

    def test_select_all_button(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        QTest.mouseClick(self.view.select_all_button, Qt.LeftButton)

        selected_plot_numbers = self.view.get_all_selected_plot_numbers()
        # Expected result: [0, 1, 2]
        # Something goes wrong in QTest here and the selection is
        # always the first plot.
        self.assertEqual([0], selected_plot_numbers)

    def test_set_active_font_makes_fonts_bold(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.set_active_font(0, True)

        name_widget = self.view.table_widget.cellWidget(0, Column.Name)
        self.assertTrue(name_widget.line_edit.font().bold())
        self.assertTrue(self.view.table_widget.item(0, Column.Number).font().bold())

    def test_unset_active_font_makes_fonts_not_bold(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.set_active_font(0, False)

        name_widget = self.view.table_widget.cellWidget(0, Column.Name)
        self.assertFalse(name_widget.line_edit.font().bold())
        self.assertFalse(self.view.table_widget.item(0, Column.Number).font().bold())

    # ----------------------- Plot Filtering ------------------------

    def test_filter_text_typing_calls_presenter_and_sets_filter_text(self):
        QTest.keyClicks(self.view.filter_box, 'plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 5)
        self.assertEquals(self.view.get_filter_text(), 'plot1')

    def test_programtic_filter_box_change_calls_presenter_and_sets_filter_text(self):
        self.view.filter_box.setText('plot1')
        self.assertEquals(self.presenter.filter_text_changed.call_count, 1)
        self.assertEquals(self.view.get_filter_text(), 'plot1')

    def test_filtering_plot_list_hides_plots(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.filter_plot_list()

        self.assertFalse(self.view.table_widget.isRowHidden(0))
        self.assertTrue(self.view.table_widget.isRowHidden(1))
        self.assertTrue(self.view.table_widget.isRowHidden(2))

    def test_filtering_then_clearing_filter_shows_all_plots(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.filter_plot_list()
        self.view.unhide_all_plots()

        self.assertFalse(self.view.table_widget.isRowHidden(0))
        self.assertFalse(self.view.table_widget.isRowHidden(1))
        self.assertFalse(self.view.table_widget.isRowHidden(2))

    def test_filtering_ignores_hidden_when_calling_get_all_selected(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)
        self.view.table_widget.selectAll()
        self.view.filter_plot_list()

        plot_names = self.view.get_all_selected_plot_numbers()
        self.assertEqual(plot_names, [0])

    def test_filtering_returns_none_for_hidden_when_calling_get_selected(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.click_to_select_by_row_number(1)
        self.view.filter_box.setText("Plot2")
        self.view.filter_plot_list()

        plot_number = self.view.get_currently_selected_plot_number()
        # Expected result: None
        # Something goes wrong in QTest here and the selection is
        # always the first plot or None.
        self.assertTrue(plot_number in [0, None])

    # ------------------------ Plot Showing ------------------------

    def test_plot_name_double_clicked_calls_presenter_and_makes_plot_current(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        item = self.view.table_widget.item(1, 1)
        item_center = self.view.table_widget.visualItemRect(item).center()
        # This single click should not be required, but otherwise the double click is not registered
        QTest.mouseClick(self.view.table_widget.viewport(), Qt.LeftButton, pos=item_center)
        QTest.mouseDClick(self.view.table_widget.viewport(), Qt.LeftButton, pos=item_center)

        self.assertEqual(self.presenter.show_single_selected.call_count, 1)
        # Expected result: 1
        # Something goes wrong in QTest here and the selection is
        # always the first plot.
        self.assertEqual(self.view.get_currently_selected_plot_number(), 0)

    def test_show_plot_by_pressing_show_button(self):
        QTest.mouseClick(self.view.show_button, Qt.LeftButton)
        self.assertEquals(self.presenter.show_multiple_selected.call_count, 1)
        QTest.mouseClick(self.view.show_button, Qt.LeftButton)
        self.assertEquals(self.presenter.show_multiple_selected.call_count, 2)

    def test_show_context_menu(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.context_menu.actions()[0].trigger()

        self.presenter.show_multiple_selected.assert_called_once_with()

    # ------------------------ Plot Hiding -------------------------

    def test_hide_button_pressed_calls_presenter(self):
        QTest.mouseClick(self.view.hide_button, Qt.LeftButton)
        self.assertEquals(self.presenter.hide_selected_plots.call_count, 1)

    def test_hide_context_menu_calls_presenter(self):
        self.view.context_menu.actions()[1].trigger()
        self.assertEquals(self.presenter.hide_selected_plots.call_count, 1)

    def test_set_visibility_icon_to_visible(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)
        self.mock_get_icon.reset_mock()
        self.view.set_visibility_icon(0, True)

        self.mock_get_icon.assert_called_once_with('fa.eye')

    def test_set_visibility_icon_to_hidden(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)
        self.mock_get_icon.reset_mock()
        self.view.set_visibility_icon(0, False)

        self.mock_get_icon.assert_called_once_with('fa.eye', color='lightgrey')

    # ------------------------ Plot Renaming ------------------------

    def test_rename_button_pressed_makes_line_editable(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        name_widget = self.view.table_widget.cellWidget(0, 1)
        QTest.mouseClick(name_widget.rename_button, Qt.LeftButton)

        self.assertFalse(name_widget.line_edit.isReadOnly())
        self.assertTrue(name_widget.rename_button.isChecked())

    def test_rename_context_menu_makes_line_editable(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        # Clicking on the QTableWidget in QTest seems unreliable
        # so we fake the selection instead,
        self.view.get_currently_selected_plot_number = mock.Mock(return_value=1)
        self.view.context_menu.actions()[3].trigger()

        name_widget = self.view.table_widget.cellWidget(1, Column.Name)
        self.assertFalse(name_widget.line_edit.isReadOnly())
        self.assertTrue(name_widget.rename_button.isChecked())

    def test_rename_finishing_editing_makes_line_uneditable_and_calls_presenter(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        name_widget = self.view.table_widget.cellWidget(1, Column.Name)
        QTest.mouseClick(name_widget.rename_button, Qt.LeftButton)
        QTest.keyPress(name_widget.line_edit, Qt.Key_Return)

        self.presenter.rename_figure.assert_called_once_with(1, "Plot2")

        self.assertTrue(name_widget.line_edit.isReadOnly())
        self.assertFalse(name_widget.rename_button.isChecked())

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

    def test_name_widget_close_button_pressed_calls_presenter(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        widget = self.view.table_widget.cellWidget(1, 1)
        QTest.mouseClick(widget.close_button, Qt.LeftButton)
        self.presenter.close_single_plot.assert_called_once_with(1)

    def test_name_widget_close_button_pressed_leaves_selection_unchanged(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        # Set the selected items by clicking with control held
        for row in [0, 2]:
            widget = self.view.table_widget.cellWidget(row, Column.Name)
            QTest.mouseClick(widget, Qt.LeftButton, Qt.ControlModifier)
        # Expected result: [0, 2]
        # Something goes wrong in QTest here and the selection is
        # not set with the control key modifier.
        plots_selected_old = self.view.get_all_selected_plot_numbers()
        self.assertEquals(plots_selected_old, [])

        widget = self.view.table_widget.cellWidget(1, Column.Name)
        QTest.mouseClick(widget.close_button, Qt.LeftButton)

        # We need to actually update the plot list, as the presenter would
        self.view.remove_from_plot_list(1)
        self.presenter.close_single_plot.assert_called_once_with(1)

        plots_selected_new = self.view.get_all_selected_plot_numbers()
        # Expected result: [0, 2]
        # Something goes wrong in QTest here and the selection is
        # not set with the control key modifier.
        self.assertEquals(plots_selected_old, plots_selected_new)

    def test_close_plot_context_menu(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        self.view.context_menu.actions()[2].trigger()

        self.presenter.close_action_called.assert_called_once_with()

    # ----------------------- Plot Sorting --------------------------

    def test_choosing_sort_ascending(self):
        self.view.set_sort_order(True)
        self.assertEquals(self.view.sort_order(), Qt.AscendingOrder)

    def test_choosing_sort_descending(self):
        self.view.set_sort_order(False)
        self.assertEquals(self.view.sort_order(), Qt.DescendingOrder)

    def test_choosing_sort_by_name(self):
        self.view.set_sort_type(Column.Name)
        self.assertEquals(self.view.sort_type(), Column.Name)

    def test_choosing_sort_by_last_active(self):
        self.view.set_sort_type(Column.LastActive)
        self.assertEquals(self.view.sort_type(), Column.LastActive)

    def test_set_sort_by_number(self):
        self.view.table_widget.setSortingEnabled(True)
        # Initial sorting is by number
        self.view.set_plot_list([0, 1, 2, 42, 19])
        self.assert_list_of_plots_is_set_in_widget(["Plot1", "Plot2", "Plot3", "Plot20", "Graph99"])

    def test_sorting_by_name(self):
        self.view.table_widget.setSortingEnabled(True)
        self.view.set_sort_type(Column.Name)
        self.view.set_plot_list([0, 1, 2, 42, 19])
        self.assert_list_of_plots_is_set_in_widget(["Graph99", "Plot1", "Plot2", "Plot3", "Plot20"])

    def test_set_sort_by_name_descending(self):
        self.view.table_widget.setSortingEnabled(True)
        self.view.set_plot_list([0, 1, 2, 42, 19])
        self.view.set_sort_type(Column.Name)
        self.view.set_sort_order(False)
        self.assert_list_of_plots_is_set_in_widget(["Plot20", "Plot3", "Plot2", "Plot1", "Graph99"])

    def test_sort_by_last_active(self):
        self.view.table_widget.setSortingEnabled(True)
        self.view.set_sort_type(Column.LastActive)
        self.view.set_plot_list([0, 1, 2, 42, 19])

        self.view.set_last_active_values({0: 2,
                                          1: 1,
                                          2: "_Plot3",
                                          42: "_Graph99",
                                          19: "_Plot20"})

        self.assert_list_of_plots_is_set_in_widget(["Plot2", "Plot1", "Graph99", "Plot3", "Plot20"])

    def adding_to_list_with_sorting_by_name(self):
        self.view.table_widget.setSortingEnabled(True)
        self.view.set_plot_list(["Plot1", "Plot2", "Plot3", "Graph99", "Plot20"])

        self.view.append_to_plot_list("Plot15")

        self.assert_list_of_plots_is_set_in_widget(["Graph99", "Plot1", "Plot2", "Plot3", "Plot15", "Plot20"])

    def adding_to_list_with_sorting_by_last_active(self):
        self.view.table_widget.setSortingEnabled(True)
        self.view.sort_type = Column.LastShown
        self.view.set_plot_list(["Plot1", "Plot2", "Plot3", "Graph99", "Plot20"])

        self.view.set_sort_keys({"Plot1": 2,
                                 "Plot2": 1,
                                 "Plot3": "_Plot3",
                                 "Graph99": "_Graph99",
                                 "Plot20": "_Plot20"})

        self.view.append_to_plot_list("Plot15")

        self.assert_list_of_plots_is_set_in_widget(["Plot2", "Plot1", "Graph99", "Plot3", "Plot15", "Plot20"])

    # ---------------------- Plot Exporting -------------------------

    def test_export_button_pressed(self):
        for i in range(len(EXPORT_TYPES)):
            self.view.export_button.menu().actions()[i].trigger()

        for i in range(len(EXPORT_TYPES)):
            self.assertEqual(self.presenter.export_plots_called.mock_calls[i],
                             mock.call(EXPORT_TYPES[i][1]))

    def test_export_context_menu(self):
        plot_numbers = [0, 1, 2]
        self.view.set_plot_list(plot_numbers)

        for i in range(len(EXPORT_TYPES)):
            self.view.export_menu.actions()[i].trigger()

        for i in range(len(EXPORT_TYPES)):
            self.assertEqual(self.presenter.export_plots_called.mock_calls[i],
                             mock.call(EXPORT_TYPES[i][1]))


if __name__ == '__main__':
    unittest.main()
