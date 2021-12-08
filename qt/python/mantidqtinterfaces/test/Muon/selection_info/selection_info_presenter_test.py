# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.selection_info_presenter import SelectionInfoPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.selection_info_view import SelectionInfoView


class SelectionInfoPresenterTest(unittest.TestCase):

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.selection_info_presenter.SelectionInfoView")
    def setUp(self, mock_view):
        self._view = mock_view
        self._view.return_value = mock.MagicMock(autospec=SelectionInfoView)
        self.context = mock.Mock()
        self.parent = mock.Mock()
        self.presenter = SelectionInfoPresenter(self.context, parent=self.parent)
        self.count = 0

    @property
    def view(self):
        return self._view.return_value

    def test_setup(self):
        self.view.hide.assert_called_once_with()
        self.view.show.assert_not_called()

    def test_set_selected_rows_from_name(self):
        self.view.selection_table.get_names_and_rows.return_value = {"one":0, "two":1, "three":2}
        self.presenter.set_selected_rows_from_name(["one", "three"])

        self.view.selection_table.set_selected_rows.assert_called_once_with([0,2])

    def test_update_lines(self):
        ws_list = ["one", "two", "three"]
        self.presenter.handle_selected_workspaces_changed = mock.Mock(return_value="unit test")
        self.assertEqual(self.presenter._lines, {})

        output = self.presenter.update_lines(ws_list,[2,1,0], ["two"])
        self.presenter.handle_selected_workspaces_changed.assert_called_once_with(ws_list, ["two"])
        self.assertEqual(output, "unit test")
        self.assertEqual(self.presenter._lines, {"one":2, "two":1, "three":0})

    def test_handle_selected_workspaces_changed(self):
        runs = [62260, 62260, 62260, 62260, 62259, 62259, 62259]
        groups = ["fwd", "bwd", "top" "bot","fwd", "bwd", "long"]
        selection = ["60f", "59f"]
        self.view.selection_table.get_selected_rows.return_value = selection
        self.presenter.get_runs_groups_and_pairs = mock.MagicMock(return_value=(runs, groups))
        self.presenter.set_selected_rows_from_name = mock.Mock()
        self.presenter.get_selection = mock.Mock(return_value = "unit test")

        names=["60f", "60b", "60u", "60bo", "59f", "59b", "59l"]
        output = self.presenter.handle_selected_workspaces_changed(names, selection)

        self.view.selection_table.set_workspaces.assert_called_once_with(names, runs, groups)
        self.presenter.set_selected_rows_from_name.assert_called_once_with(selection)
        self.view.selection_table.set_selection_to_last_row.assert_not_called()
        self.assertEqual(output, "unit test")

    def test_handle_selected_workspaces_changed_no_selection(self):
        runs = [62260, 62260, 62260, 62260, 62259, 62259, 62259]
        groups = ["fwd", "bwd", "top" "bot","fwd", "bwd", "long"]
        selection = []
        self.view.selection_table.get_selected_rows.return_value = selection
        self.presenter.get_runs_groups_and_pairs = mock.MagicMock(return_value=(runs, groups))
        self.presenter.set_selected_rows_from_name = mock.Mock()
        self.presenter.get_selection = mock.Mock(return_value = "unit test")

        names=["60f", "60b", "60u", "60bo", "59f", "59b", "59l"]
        output = self.presenter.handle_selected_workspaces_changed(names, selection)

        self.view.selection_table.set_workspaces.assert_called_once_with(names, runs, groups)
        self.presenter.set_selected_rows_from_name.assert_not_called()
        self.view.selection_table.set_selection_to_last_row.assert_called_once_with()
        self.assertEqual(output, "unit test")

    def test_get_selection(self):
        self.view.selection_table.get_selected_rows.return_value = [0,2]

        def get_names(index):
            names = ["60f", "60b", "59f", "59b"]
            return names[index]
        self.presenter._lines = {"60f":0, "60b":1, "59f":2, "59b":3}
        self.view.selection_table.get_workspace_names_from_row.side_effect = get_names

        self.assertEqual(self.presenter.get_selection(), {"60f":0, "59f":2})

    def test_get_runs_from_groups_and_pairs(self):
        # needs valid muon names
        self.context._data_context = mock.Mock()
        type(self.context._data_context).instrument = mock.PropertyMock(return_value="MUSR")
        ws_names = ["MUSR62260; Group; fwd; Counts; MA",
                    "MUSR62260; Group; bwd; Counts; MA",
                    "MUSR62259; Group; fwd; Counts; MA"]
        runs, groups = self.presenter.get_runs_groups_and_pairs(ws_names)
        self.assertEqual(runs, ["62260", "62260", "62259"])
        self.assertEqual(groups, ["fwd", "bwd", "fwd"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
