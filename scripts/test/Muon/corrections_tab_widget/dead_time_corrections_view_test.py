# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_view import DeadTimeCorrectionsView

from qtpy.QtWidgets import QApplication


@start_qapplication
class DeadTimeCorrectionsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = DeadTimeCorrectionsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_the_workspace_selector_and_other_file_options_invisible(self):
        self.assertTrue(self.view.dead_time_workspace_label.isHidden())
        self.assertTrue(self.view.dead_time_workspace_selector.isHidden())
        self.assertTrue(self.view.dead_time_other_file_label.isHidden())
        self.assertTrue(self.view.dead_time_browse_button.isHidden())

    def test_that_set_dead_time_workspace_selector_visible_will_make_the_workspace_selector_visible(self):
        self.view.set_dead_time_workspace_selector_visible(True)

        self.assertTrue(not self.view.dead_time_workspace_label.isHidden())
        self.assertTrue(not self.view.dead_time_workspace_selector.isHidden())

    def test_that_set_dead_time_other_file_visible_will_make_the_workspace_selector_visible(self):
        self.view.set_dead_time_other_file_visible(True)

        self.assertTrue(not self.view.dead_time_other_file_label.isHidden())
        self.assertTrue(not self.view.dead_time_browse_button.isHidden())

    def test_that_the_view_is_initialized_with_the_from_data_file_option_selected(self):
        self.assertTrue(self.view.is_dead_time_from_data_file_selected())
        self.assertTrue(not self.view.is_dead_time_from_workspace_selected())
        self.assertTrue(not self.view.is_dead_time_from_other_file_selected())

    def test_that_set_dead_time_from_workspace_selected_will_select_the_from_workspace_option(self):
        self.view.set_dead_time_from_workspace_selected()

        self.assertTrue(not self.view.is_dead_time_from_data_file_selected())
        self.assertTrue(self.view.is_dead_time_from_workspace_selected())
        self.assertTrue(not self.view.is_dead_time_from_other_file_selected())

    def test_that_populate_dead_time_workspace_selector_will_populate_the_workspace_selector(self):
        self.view.populate_dead_time_workspace_selector(["Table1", "Table2"])

        items = [self.view.dead_time_workspace_selector.itemText(i)
                 for i in range(self.view.dead_time_workspace_selector.count())]
        expected_items = ["None", "Table1", "Table2"]
        self.assertEqual(items, expected_items)

    def test_that_selected_dead_time_workspace_will_return_none_by_default(self):
        self.view.set_dead_time_from_workspace_selected()
        self.view.populate_dead_time_workspace_selector(["Table1", "Table2"])

        self.assertEqual(self.view.selected_dead_time_workspace(), "None")

    def test_that_set_selected_dead_time_workspace_will_set_which_dead_time_workspace_is_selected(self):
        self.view.set_dead_time_from_workspace_selected()
        self.view.populate_dead_time_workspace_selector(["Table1", "Table2"])

        self.view.set_selected_dead_time_workspace("Table1")
        self.assertEqual(self.view.selected_dead_time_workspace(), "Table1")

    def test_that_populate_dead_time_workspace_selector_will_reselect_the_previously_selected_workspace(self):
        self.view.set_dead_time_from_workspace_selected()
        self.view.populate_dead_time_workspace_selector(["Table1", "Table2"])
        self.view.set_selected_dead_time_workspace("Table1")

        self.view.populate_dead_time_workspace_selector(["Table1", "Table2", "Table3"])

        self.assertEqual(self.view.selected_dead_time_workspace(), "Table1")

    def test_that_set_dead_time_info_text_will_set_the_label_text_to_the_expected_value(self):
        message = "This is a test message."
        self.view.set_dead_time_info_text(message)

        self.assertEqual(self.view.dead_time_stats_label.text(), "")
        self.assertEqual(self.view.dead_time_info_label.text(), message)

    def test_that_set_dead_time_average_and_range_will_set_the_label_to_the_average_dead_time(self):
        average = 2.0021
        limits = (1.0011, 3.0031)
        self.view.set_dead_time_average_and_range("84447", limits, average)

        self.assertEqual(self.view.dead_time_stats_label.text(), "Dead Time stats for run 84447:")
        self.assertEqual(self.view.dead_time_info_label.text(), "1.001 to 3.003 (av. 2.002)")


if __name__ == '__main__':
    unittest.main()
