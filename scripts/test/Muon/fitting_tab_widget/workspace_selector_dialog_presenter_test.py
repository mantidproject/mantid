# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class WorkspaceSelectorPresenterTest(GuiTest):
    def setUp(self):
        self.current_runs = [[22725]]
        self.context = setup_context()
        self.context.get_names_of_workspaces_to_fit = mock.MagicMock(return_value=['MUSR22725; Group; fwd; Asymmetry; #1'])
        self.view = WorkspaceSelectorView(self.current_runs, 'MUSR', [], True, self.context)
        self.view.list_selector_presenter = mock.MagicMock()
        self.context.get_names_of_workspaces_to_fit.reset_mock()

    def test_handle_group_pair_selection_changed(self):
        self.view.group_pair_line_edit.setText('fwd, bwd')
        self.view.group_pair_line_edit.editingFinished.emit()

        self.context.get_names_of_workspaces_to_fit.assert_any_call(group_and_pair='fwd, bwd', phasequad=False,
                                                                            rebin=False, runs='All', freq='None')

        self.context.get_names_of_workspaces_to_fit.assert_any_call(group_and_pair='All', phasequad=True,
                                                                           rebin=False, runs='All', freq='None')

        self.view.list_selector_presenter.update_model.assert_not_called()
        self.view.list_selector_presenter.update_filter_list.assert_called_once_with([])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
