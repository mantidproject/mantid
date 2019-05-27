# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from Muon.GUI.Common.results_tab_widget.results_tab_widget import ResultsTabWidget
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantid.api import FunctionFactory


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class ResultsTabPresenterTest(GuiTest):
    def setUp(self):
        self.context = setup_context()
        self.results_tab_widget = ResultsTabWidget(self.context, None)
        self.view = self.results_tab_widget.results_tab_view
        self.presenter = self.results_tab_widget.results_tab_presenter

    def test_that_widget_initialises_correctly(self):
        self.assertEqual(self.presenter.get_selected_fit_list(), [])
        self.assertEqual(self.presenter.get_selected_logs_list(), [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
