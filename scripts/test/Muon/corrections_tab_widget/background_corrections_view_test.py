# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.corrections_tab_widget.background_corrections_view import (BackgroundCorrectionsView,
                                                                                RUN_COLUMN_INDEX,
                                                                                GROUP_COLUMN_INDEX,
                                                                                USE_RAW_COLUMN_INDEX,
                                                                                BG_COLUMN_INDEX,
                                                                                BG_ERROR_COLUMN_INDEX,
                                                                                STATUS_COLUMN_INDEX)

from qtpy.QtWidgets import QApplication


@start_qapplication
class BackgroundCorrectionsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = BackgroundCorrectionsView()
        self.view.show()
        self.assert_widget_created()

        self.runs = ["84447", "84447", "84447", "84447"]
        self.groups = ["fwd", "bwd", "top", "bottom"]
        self.use_raws = [True, True, True, True]
        self.start_xs = [5.0, 6.0, 7.0, 8.0]
        self.end_xs = [9.0, 10.0, 11.0, 12.0]
        self.a0s = [0.0, 0.0, 0.1, 0.1]
        self.a0_errors = [0.0, 0.0, 0.0, 0.0]
        self.statuses = ["Corrections success", "Corrections success", "Corrections success", "Corrections skipped"]
        self.use_raw_enabled = False

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_most_background_correction_options_invisible(self):
        self.assertTrue(self.view.select_function_label.isHidden())
        self.assertTrue(self.view.function_combo_box.isHidden())
        self.assertTrue(self.view.show_data_for_label.isHidden())
        self.assertTrue(self.view.group_combo_box.isHidden())
        self.assertTrue(self.view.show_all_runs_checkbox.isHidden())
        self.assertTrue(self.view.apply_table_changes_to_all_checkbox.isHidden())
        self.assertTrue(self.view.correction_options_table.isHidden())

    def test_that_set_background_correction_options_visible_will_make_the_correction_options_visible(self):
        self.view.set_background_correction_options_visible(True)

        self.assertTrue(not self.view.select_function_label.isHidden())
        self.assertTrue(not self.view.function_combo_box.isHidden())
        self.assertTrue(not self.view.show_data_for_label.isHidden())
        self.assertTrue(not self.view.group_combo_box.isHidden())
        self.assertTrue(not self.view.show_all_runs_checkbox.isHidden())
        self.assertTrue(not self.view.apply_table_changes_to_all_checkbox.isHidden())
        self.assertTrue(not self.view.correction_options_table.isHidden())

    def test_that_background_correction_mode_will_return_and_set_the_correction_mode_as_expected(self):
        self.assertEqual(self.view.background_correction_mode, "None")

        self.view.background_correction_mode = "Auto"
        self.assertEqual(self.view.background_correction_mode, "Auto")

    def test_that_background_correction_mode_will_not_raise_if_the_option_provided_does_not_exist(self):
        self.assertEqual(self.view.background_correction_mode, "None")

        self.view.background_correction_mode = "FakeMode"
        self.assertEqual(self.view.background_correction_mode, "None")

    def test_that_selected_function_will_return_and_set_the_selected_function_as_expected(self):
        self.assertEqual(self.view.selected_function, "Flat Background + Exp Decay")

        self.view.selected_function = "Flat Background"
        self.assertEqual(self.view.selected_function, "Flat Background")

    def test_that_selected_function_will_not_raise_if_the_function_option_provided_does_not_exist(self):
        self.assertEqual(self.view.selected_function, "Flat Background + Exp Decay")

        self.view.selected_function = "Fake Function"
        self.assertEqual(self.view.selected_function, "Flat Background + Exp Decay")

    def test_that_apply_table_changes_to_all_returns_true_by_default_when_view_is_initialized(self):
        self.assertTrue(self.view.apply_table_changes_to_all())

    def test_that_apply_table_changes_to_all_returns_false_when_the_checkbox_is_unchecked(self):
        self.view.apply_table_changes_to_all_checkbox.setChecked(False)
        self.assertTrue(not self.view.apply_table_changes_to_all())

    def test_that_is_run_group_displayed_returns_false_if_a_run_and_group_does_not_exist(self):
        self.assertTrue(not self.view.is_run_group_displayed("FakeRun", "FakeGroup"))

    def test_that_is_run_group_displayed_returns_true_if_a_run_and_group_does_not_exist(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)
        for run, group in zip(self.runs, self.groups):
            self.assertTrue(self.view.is_run_group_displayed(run, group))

    def test_that_populate_group_selector_will_populate_the_group_selector_with_the_first_entry_being_all(self):
        self.view.populate_group_selector(self.groups)

        self.assertEqual([self.view.group_combo_box.itemText(i) for i in range(self.view.group_combo_box.count())],
                         ["All"] + self.groups)

    def test_that_selected_group_returns_the_selected_group_from_the_group_combo_box(self):
        self.view.populate_group_selector(self.groups)

        self.view.group_combo_box.setCurrentIndex(2)

        self.assertEqual(self.view.selected_group, "bwd")

    def test_that_populate_corrections_table_will_display_the_data_provided_in_the_table(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        for row_i in range(self.view.correction_options_table.rowCount()):
            self.assertEqual(self.view.correction_options_table.item(row_i, RUN_COLUMN_INDEX).text(), self.runs[row_i])
            self.assertEqual(self.view.correction_options_table.item(row_i, GROUP_COLUMN_INDEX).text(),
                             self.groups[row_i])
            self.assertEqual(self.view.correction_options_table.item(row_i, USE_RAW_COLUMN_INDEX).checkState(),
                             Qt.Checked)
            self.assertEqual(self.view.start_x(self.runs[row_i], self.groups[row_i]), self.start_xs[row_i])
            self.assertEqual(self.view.end_x(self.runs[row_i], self.groups[row_i]), self.end_xs[row_i])
            self.assertEqual(self.view.correction_options_table.item(row_i, BG_COLUMN_INDEX).text(),
                             f"{self.a0s[row_i]:.3f}")
            self.assertEqual(self.view.correction_options_table.item(row_i, BG_ERROR_COLUMN_INDEX).text(),
                             f"{self.a0_errors[row_i]:.3f}")
            self.assertEqual(self.view.correction_options_table.item(row_i, STATUS_COLUMN_INDEX).text(),
                             self.statuses[row_i])

    def test_that_set_start_x_will_set_the_start_x_for_a_specific_run_and_group(self):
        run = "84447"
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        self.view.set_start_x(run, "fwd", 1.1)
        self.view.set_start_x(run, "bwd", 2.2)
        self.view.set_start_x(run, "top", 3.3)
        self.view.set_start_x(run, "bottom", 4.4)

        self.assertEqual(self.view.start_x(run, "fwd"), 1.1)
        self.assertEqual(self.view.start_x(run, "bwd"), 2.2)
        self.assertEqual(self.view.start_x(run, "top"), 3.3)
        self.assertEqual(self.view.start_x(run, "bottom"), 4.4)

    def test_that_set_end_x_will_set_the_end_x_for_a_specific_run_and_group(self):
        run = "84447"
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        self.view.set_end_x(run, "fwd", 1.1)
        self.view.set_end_x(run, "bwd", 2.2)
        self.view.set_end_x(run, "top", 3.3)
        self.view.set_end_x(run, "bottom", 4.4)

        self.assertEqual(self.view.end_x(run, "fwd"), 1.1)
        self.assertEqual(self.view.end_x(run, "bwd"), 2.2)
        self.assertEqual(self.view.end_x(run, "top"), 3.3)
        self.assertEqual(self.view.end_x(run, "bottom"), 4.4)

    def test_that_selected_run_and_group_will_raise_if_there_is_no_group_selected(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)
        self.assertRaises(RuntimeError, self.view.selected_run_and_group)

    def test_that_selected_run_and_group_will_return_the_run_and_group_of_the_selected_row(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        self._select_table_cell(2, 0)
        self.assertEqual(self.view.selected_run_and_group(), (["84447"], ["top"]))

    def test_that_selected_start_x_will_raise_if_there_is_no_row_selected(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)
        self.assertRaises(RuntimeError, self.view.selected_start_x)

    def test_that_selected_start_x_will_return_the_start_x_of_the_selected_row(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        self._select_table_cell(2, 0)
        self.assertEqual(self.view.selected_start_x(), 7.0)

    def test_that_selected_end_x_will_raise_if_there_is_no_row_selected(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)
        self.assertRaises(RuntimeError, self.view.selected_end_x)

    def test_that_selected_end_x_will_return_the_end_x_of_the_selected_row(self):
        self.view.populate_corrections_table(self.runs, self.groups, self.use_raws, self.start_xs, self.end_xs, self.a0s,
                                             self.a0_errors, self.statuses, self.use_raw_enabled)

        self._select_table_cell(2, 0)
        self.assertEqual(self.view.selected_end_x(), 11.0)

    def _select_table_cell(self, row, column):
        x = int(self.view.correction_options_table.columnViewportPosition(column)
                + self.view.correction_options_table.columnWidth(column) / 2)
        y = int(self.view.correction_options_table.rowViewportPosition(row)
                + self.view.correction_options_table.rowHeight(row) / 2)

        QTest.mouseClick(self.view.correction_options_table.viewport(), Qt.LeftButton, Qt.NoModifier, QPoint(x, y))


if __name__ == '__main__':
    unittest.main()
