# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_view import QuickEditView
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_presenter import QuickEditPresenter
from Muon.GUI.Common.contexts.plotting_context import PlottingContext

from mantid.simpleapi import AnalysisDataService
from mantidqt.utils.qt.testing import start_qapplication


def plot_at_index(index):
    return "plot "+str(index)


@start_qapplication
class QuickEditTest(unittest.TestCase):

    def setUp(self):
        self.view = QuickEditView(None, None)
        self.context = PlottingContext()
        self.presenter = QuickEditPresenter(self.view, self.context)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def add_plots(self, num_plots):
        for i in range(1, num_plots+1):
            self.view.plot_selector.addItem("Plot " + str(i))

    def test_add_subplot(self):
        self.view.set_selection = mock.Mock()
        self.assertEqual(1, self.view.plot_selector.count())
        self.presenter.add_subplot("Plot 1")
        self.assertEqual(2, self.view.plot_selector.count())
        self.view.set_selection.assert_called_once_with(0)  # Selection should not change

    def test_clear_subplots(self):
        self.add_plots(3)
        self.presenter.clear_subplots()
        self.assertEqual(1, self.view.plot_selector.count())  # Expect All plot to still be there

    def test_all(self):
        self.add_plots(3)
        expected = ["Plot 1", "Plot 2", "Plot 3"]
        self.assertEqual(expected, self.presenter.get_all_subplots)

    def test_set_errors_same_as_previous(self):
        self.view.get_errors = mock.Mock(return_value=False)
        self.view.set_errors = mock.Mock()
        self.presenter.set_errors(False)
        self.view.set_errors.assert_not_called()

    def test_set_errors_not_same_as_previous(self):
        self.view.get_errors = mock.Mock(return_value=False)
        self.view.set_errors = mock.Mock()
        self.presenter.set_errors(True)
        self.view.set_errors.assert_called_once_with(True)

    def test_get_selection_all(self):
        self.add_plots(2)
        self.view.current_selection = mock.Mock(return_value="All")
        expected = ["Plot 1", "Plot 2"]
        self.assertEqual(expected, self.presenter.get_selection())

    def test_get_selection(self):
        self.add_plots(2)
        self.view.current_selection = mock.Mock(return_value="Plot 1")
        self.assertEqual(["Plot 1"], self.presenter.get_selection())

    def test_remove_subplot(self):
        self.add_plots(2)
        self.view.plot_selector.currentText = mock.Mock(return_value="Plot 2")
        self.view.set_selection = mock.Mock()
        self.view.rm_subplot = mock.Mock()
        self.presenter.rm_subplot("Plot 1")
        self.view.rm_subplot.assert_called_once_with(1)

    def test_remove_current_subplot(self):
        self.add_plots(2)
        self.view.current_selection = mock.Mock(return_value="Plot 2")
        self.view.set_selection = mock.Mock()
        self.view.rm_subplot = mock.Mock()
        self.presenter.rm_subplot("Plot 2")
        self.view.rm_subplot.assert_called_once_with(2)
        self.view.set_selection.assert_called_once_with(0)  # Set to all

    def test_multiple_plots(self):
        self.view.number_of_plots = mock.Mock(return_value=4)
        self.view.plot_at_index = mock.Mock(side_effect = plot_at_index)
        # plot  name at index 0 is reserved so its excluded
        expected = ["plot 1", "plot 2", "plot 3"]
        self.assertEqual(self.presenter.multiple_plots(), expected)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
