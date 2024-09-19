# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.sans_isis.gui_logic.models.RunSelectionModel import RunSelectionModel
from mantidqtinterfaces.sans_isis.gui_logic.models.run_file import SummableRunFile
from mantidqtinterfaces.sans_isis.gui_logic.models.run_finder import SummableRunFinder
from mantidqtinterfaces.sans_isis.gui_logic.presenter.RunSelectorPresenter import RunSelectorPresenter
from mantidqtinterfaces.sans_isis.views.run_selector_widget import RunSelectorWidget
from fake_signal import FakeSignal


class RunSelectorPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.run_selection = self._make_mock_selection()
        self.run_finder = self._make_mock_finder()
        self.presenter = self._make_presenter(self.run_selection, self.run_finder, self.view)

    def _make_mock_view(self):
        mock_view = mock.create_autospec(RunSelectorWidget, spec_set=True)
        mock_view.addRuns = FakeSignal()
        mock_view.removeRuns = FakeSignal()
        mock_view.manageDirectories = FakeSignal()
        mock_view.browse = FakeSignal()
        mock_view.removeAllRuns = FakeSignal()
        return mock_view

    def _make_mock_selection(self):
        return mock.create_autospec(RunSelectionModel)

    def _make_mock_finder(self):
        return mock.create_autospec(SummableRunFinder)

    def _make_presenter(self, run_selection, run_finder, view):
        return RunSelectorPresenter("some_title", run_selection, run_finder, view, None)

    def test_searches_for_runs_when_add_run_pressed(self):
        run_query = "1"
        no_runs = ("", [])
        self.view.run_list.return_value = run_query
        self.run_finder.find_all_from_query.return_value = no_runs
        self.view.addRuns.emit()
        self.run_finder.find_all_from_query.assert_called_with(run_query)

    def _make_fake_run_model(self, run_name):
        return SummableRunFile("/home/{}".format(run_name), run_name, is_event_mode=True)

    def test_adds_search_results_to_model_when_add_run_pressed(self):
        run_name = "1"
        run_query = run_name
        found_run = self._make_fake_run_model(run_name)

        self.view.run_list.return_value = run_query
        self.run_finder.find_all_from_query.return_value = ("", [found_run])

        self.view.addRuns.emit()
        self.run_selection.add_run.assert_called_with(found_run)

    def test_handles_error_when_invalid_query(self):
        run_query = "1-0"
        error_message = "Invalid Query"
        self.view.run_list.return_value = run_query
        self.run_finder.find_all_from_query.return_value = (error_message, [])

        self.view.addRuns.emit()
        self.view.invalid_run_query.assert_called_with(error_message)

    def test_handles_error_when_run_not_found(self):
        run_query = "1-10"
        self.view.run_list.return_value = run_query
        self.run_finder.find_all_from_query.return_value = ("", [])

        self.view.addRuns.emit()
        self.view.run_not_found.assert_called()

    def test_adds_multiple_search_results_to_model_when_add_run_pressed(self):
        run_names = ["1", "009", "12"]
        run_query = ",".join(run_names)
        found_runs = [self._make_fake_run_model(run_name) for run_name in run_names]

        self.view.run_list.return_value = run_query
        self.run_finder.find_all_from_query.return_value = ("", found_runs)

        self.view.addRuns.emit()
        expected = [mock.call.add_run(run) for run in found_runs]
        self.run_selection.assert_has_calls(expected)

    def test_remove_runs_removes_run_from_model(self):
        run_index = 0
        self.view.selected_runs.return_value = [run_index]
        self.view.removeRuns.emit()
        self.run_selection.remove_run.assert_called_with(run_index)

    def test_removes_runs_from_model_when_multi_selected(self):
        run_indices = [0, 2]
        self.view.selected_runs.return_value = run_indices
        self.view.removeRuns.emit()
        expected = [mock.call.remove_run(index) for index in run_indices]
        self.run_selection.assert_has_calls(expected, any_order=True)

    def test_removes_runs_in_correct_order_when_multi_selected(self):
        run_indices = [2, 1, 5, 0]
        self.view.selected_runs.return_value = run_indices
        self.view.removeRuns.emit()
        expected = [mock.call.remove_run(index) for index in [5, 2, 1, 0]]
        self.run_selection.assert_has_calls(expected, any_order=False)

    def test_clears_all_runs_from_model_when_clear_pressed(self):
        self.view.removeAllRuns.emit()
        self.run_selection.clear_all_runs.assert_called()

    def test_manage_directories_launches_dialog(self):
        self.view.manageDirectories.emit()
        self.view.show_directories_manager.assert_called()

    def test_browse_to_directory(self):
        self.view.browse.emit()
        self.view.show_file_picker.assert_called()


if __name__ == "__main__":
    unittest.main()
