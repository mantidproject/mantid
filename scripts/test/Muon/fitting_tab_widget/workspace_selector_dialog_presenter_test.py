# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class WorkspaceSelectorPresenterTest(unittest.TestCase):
    def setUp(self):
        self.current_runs = [[22725]]
        self.context = setup_context()
        self.context.get_names_of_workspaces_to_fit = mock.MagicMock(
            return_value=['MUSR22725; Group; fwd; Asymmetry; #1'])
        self.view = WorkspaceSelectorView(self.current_runs, 'MUSR', [], True, None, self.context)
        self.view.list_selector_presenter = mock.MagicMock()
        self.context.get_names_of_workspaces_to_fit.reset_mock()

    def test_handle_group_pair_selection_changed(self):
        self.view.group_pair_line_edit.setText('fwd, bwd')
        self.view.group_pair_line_edit.editingFinished.emit()

        self.context.get_names_of_workspaces_to_fit.assert_any_call(group_and_pair='fwd, bwd',
                                                                    rebin=False, runs='All', freq='None')

        self.view.list_selector_presenter.update_model.assert_not_called()
        self.view.list_selector_presenter.update_filter_list.assert_called_once_with([])

    def test_is_it_freq_flase(self):
        self.assertEquals(self.view.is_it_freq, "None")


@start_qapplication
class WorkspaceSelectorPresenterWithFrequencyTest(unittest.TestCase):
    def setUp(self):
        self.current_runs = [[22725]]
        self.context = setup_context(True)
        self.context.get_names_of_workspaces_to_fit = mock.MagicMock(
            return_value=['MUSR22725; Group; fwd; Asymmetry; #1'])
        self.view = WorkspaceSelectorView(self.current_runs, 'MUSR', [], True, None, self.context)
        self.view.list_selector_presenter = mock.MagicMock()
        self.context.get_names_of_workspaces_to_fit.reset_mock()

    def set_combo(self, name):
        index = self.view.time_domain_combo.findText(name)
        self.view.time_domain_combo.setCurrentIndex(index)

    def test_is_it_freq_flase(self):
        self.set_combo("Time Domain")
        self.assertEquals(self.view.is_it_freq, "None")

    def test_is_it_freq_true(self):
        self.set_combo("Frequency Re")
        self.assertEquals(self.view.is_it_freq, "Re")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
