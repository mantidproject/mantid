# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView

from qtpy.QtWidgets import QApplication


@start_qapplication
class CorrectionsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = CorrectionsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_is_disabled_when_initialized(self):
        self.assertTrue(not self.view.isEnabled())

    def test_that_the_view_will_not_be_enabled_if_there_are_no_runs_loaded(self):
        self.assertTrue(not self.view.isEnabled())
        self.view.enable_view()
        self.assertTrue(not self.view.isEnabled())

    def test_that_the_view_will_be_enabled_if_there_are_more_than_one_runs_loaded(self):
        self.assertTrue(not self.view.isEnabled())
        self.view.update_run_selector_combo_box(["62260"])
        self.view.enable_view()
        self.assertTrue(self.view.isEnabled())

    def test_that_update_run_selector_combo_box_will_set_the_runs_in_the_runs_selector(self):
        runs = ["62260", "62261", "62262"]
        self.view.update_run_selector_combo_box(runs)

        items = [self.view.run_selector.dataset_name_combo_box.itemText(i)
                 for i in range(self.view.run_selector.dataset_name_combo_box.count())]
        self.assertEqual(items, runs)

    def test_that_current_run_string_will_return_the_current_run_as_a_string(self):
        runs = ["62260", "62261", "62262"]
        self.view.update_run_selector_combo_box(runs)

        run_string = self.view.current_run_string()
        self.assertEqual(type(run_string), str)
        self.assertEqual(run_string, "62260")


if __name__ == '__main__':
    unittest.main()
