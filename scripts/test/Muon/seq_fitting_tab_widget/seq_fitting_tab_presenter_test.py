# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter import SeqFittingTabPresenter

from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class FittingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context()
        self.loaded_run = 62260
        self.context.data_context.current_runs = [[self.loaded_run]]
        self.context.data_context.instrument = 'MUSR'
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.presenter = SeqFittingTabPresenter(self.view, self.model, self.context)

    def test_updated_workspace_list_correctly_adds_workspaces_to_table(self):
        workspace_list = ["MUSR62260", "MUSR62261", "MUSR62262"]
        self.presenter.model.get_selected_workspace_list = mock.MagicMock(return_value=workspace_list)
        self.view.set_fit_table_workspaces = mock.MagicMock()

        self.presenter.handle_selected_workspaces_changed()
        self.view.set_fit_table_workspaces.assert_called_with(workspace_list)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
