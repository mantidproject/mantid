import unittest
import sys
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.models.add_runs_model import (AddRunsModel, SummableRunModel, BinningType)
from ui.sans_isis.add_runs_page import AddRunsPage

class FakeSignal:
    def __init__(self):
        self._handlers = []

    def connect(self, handler):
        self._handlers.append(handler)

    def emit(self, **args):
        for handler in self._handlers:
            handler(**args)

if sys.version_info.major == 3:
     from unittest import mock
else:
     import mock

class AddRunsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.model = self._make_mock_model()
        self.presenter = self._make_presenter(self.model, self.view)

    def _make_mock_view(self):
        mock_view = mock.create_autospec(AddRunsPage, spec_set=True)
        mock_view.addRuns = FakeSignal()
        mock_view.removeRuns = FakeSignal()
        mock_view.manageDirectories = FakeSignal()
        mock_view.browse = FakeSignal()
        mock_view.removeAllRuns = FakeSignal()
        mock_view.binningTypeChanged = FakeSignal()
        mock_view.preserveEventsChanged = FakeSignal()
        mock_view.sum = FakeSignal()
        return mock_view

    def _make_mock_model(self):
        return mock.create_autospec(AddRunsModel)

    def _make_presenter(self, model, view):
        return AddRunsPagePresenter(model, view, None)

    def test_searches_for_runs_when_add_run_pressed(self):
        run_query = '1'
        no_runs = ('', [])
        self.view.run_list.return_value = run_query
        self.model.find_all_from_query.return_value = no_runs
        self.view.addRuns.emit()
        self.model.find_all_from_query.assert_called_with(run_query)

    def _make_fake_run_model(self, run_name):
        return SummableRunModel('/home/{}'.format(run_name))

    def test_adds_search_results_to_model_when_add_run_pressed(self):
        run_name = '1'
        run_query = run_name
        found_run = self._make_fake_run_model(run_name)

        self.view.run_list.return_value = run_query
        self.model.find_all_from_query.return_value = ('', [found_run])

        self.view.addRuns.emit()
        self.model.add_run.assert_called_with(found_run)

    def test_handles_error_when_invalid_query(self):
        run_query = '1-0'
        error_message = 'Invalid Query'
        self.view.run_list.return_value = run_query
        self.model.find_all_from_query.return_value = (error_message, [])

        self.view.addRuns.emit()
        self.view.invalid_run_query.assert_called_with(error_message)

    def test_handles_error_when_run_not_found(self):
        run_query = '1-10'
        self.view.run_list.return_value = run_query
        self.model.find_all_from_query.return_value = ('', [])

        self.view.addRuns.emit()
        self.view.run_not_found.assert_called()

    def test_adds_multiple_search_results_to_model_when_add_run_pressed(self):
        run_names = ['1', '009', '12']
        run_query = ",".join(run_names)
        found_runs = [self._make_fake_run_model(run_name) for run_name in run_names]

        self.view.run_list.return_value = run_query
        self.model.find_all_from_query.return_value = ('', found_runs)

        self.view.addRuns.emit()
        expected = [mock.call.add_run(run) for run in found_runs]
        self.model.assert_has_calls(expected)

    def test_remove_runs_removes_run_from_model(self):
        run_index = 0
        self.view.selected_runs.return_value = [run_index]
        self.view.removeRuns.emit()
        self.model.remove_run.assert_called_with(run_index)

    def test_removes_runs_from_model_when_multi_selected(self):
        run_indices = [0, 2]
        self.view.selected_runs.return_value = run_indices
        self.view.removeRuns.emit()
        expected = [mock.call.remove_run(index) for index in run_indices]
        self.model.assert_has_calls(expected, any_order=True)

    def test_removes_runs_in_correct_order_when_multi_selected(self):
        run_indices = [2, 1, 5, 0]
        self.view.selected_runs.return_value = run_indices
        self.view.removeRuns.emit()
        expected = [mock.call.remove_run(index) for index in [5, 2, 1, 0]]
        self.model.assert_has_calls(expected, any_order=False)

    def test_clears_all_runs_from_model_when_clear_pressed(self):
        self.view.removeAllRuns.emit()
        self.model.clear_all_runs.assert_called()

    def test_manage_directories_launches_dialog(self):
        self.view.manageDirectories.emit()
        self.view.show_directories_manager.assert_called()

    def test_browse_to_directory(self):
        self.view.browse.emit()
        self.view.show_file_picker.assert_called()

if __name__ == '__main__': unittest.main()
