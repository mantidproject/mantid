import sys
import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from PyQt4.QtGui import QApplication

from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.muon_load_data import MuonLoadData


class IteratorWithException:
    """Wraps a simple iterable (i.e. list) so that it throws a ValueError on a particular index."""

    def __init__(self, iterable, throw_on_index):
        self.max = len(iterable)
        self.iterable = iter(iterable)

        self.throw_indices = [index for index in throw_on_index if index < self.max]

    def __iter__(self):
        self.n = 0
        return self

    def __next__(self):

        if self.n in self.throw_indices:
            next(self.iterable)
            self.n += 1
            raise ValueError()
        elif self.n == self.max:
            raise StopIteration()
        else:
            self.n += 1
            return next(self.iterable)

    next = __next__


# global QApplication (get errors if > 1 instance in the code)
QT_APP = QApplication([])


class LoadFileWidgetPresenterTest(unittest.TestCase):
    def run_test_with_and_without_threading(test_function):

        def run_twice(self):
            test_function(self)
            self.setUp()
            self.presenter._use_threading = False
            test_function(self)

        return run_twice

    class Runner:

        def __init__(self, thread):
            self.QT_APP = QT_APP
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                if self._thread.isRunning():
                    self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        self.view = BrowseFileWidgetView()

        self.view.on_browse_clicked = mock.Mock()
        self.view.set_file_edit = mock.Mock()
        self.view.reset_edit_to_cached_value = mock.Mock()
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.data = MuonLoadData()
        self.model = BrowseFileWidgetModel(self.data)
        self.model.exception_message_for_failed_files = mock.Mock()

        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()
        self.view.warning_popup = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.view, self.model)
        self.presenter.enable_multiple_files(False)

    def mock_browse_button_to_return_files(self, files):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=files)

    def mock_user_input_text(self, text):
        self.view.get_file_edit_text = mock.Mock(return_value=text)

    def mock_model_to_load_workspaces(self, workspaces, runs):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=zip(workspaces, runs))

    def load_workspaces_into_model_and_view_from_browse(self, workspaces, runs, files):
        self.mock_model_to_load_workspaces(workspaces, runs)
        self.mock_browse_button_to_return_files(files)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_browser_dialog_opens_when_browse_button_clicked(self):
        self.mock_browse_button_to_return_files(["file.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.show_file_browser_and_return_selection.call_count, 1)

    @run_test_with_and_without_threading
    def test_loading_not_initiated_if_no_file_selected_from_browser(self):
        self.mock_model_to_load_workspaces([], [])
        self.mock_browse_button_to_return_files([])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.load_workspace_from_filename.call_count, 0)

    @run_test_with_and_without_threading
    def test_buttons_disabled_while_load_thread_running(self):
        self.mock_browse_button_to_return_files(["file.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.view.enable_load_buttons.call_count, 1)

    @run_test_with_and_without_threading
    def test_buttons_enabled_after_load_even_if_load_thread_throws(self):
        self.model.execute = mock.Mock(side_effect=self.load_failure)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.disable_load_buttons.call_count,
                         self.view.enable_load_buttons.call_count)

    @run_test_with_and_without_threading
    def test_files_not_loaded_into_model_if_multiple_files_selected_from_browse_in_single_file_mode(self):
        self.mock_model_to_load_workspaces([[1], [2]], [1234, 1235])
        self.mock_browse_button_to_return_files(["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.model.execute = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.execute.call_count, 0)
        self.assertEqual(self.view.disable_load_buttons.call_count, 0)
        self.assertEqual(self.view.enable_load_buttons.call_count, 0)

    @run_test_with_and_without_threading
    def test_files_not_loaded_into_model_if_multiple_files_entered_by_user_in_single_file_mode(self):
        self.mock_user_input_text("C:/dir1/file1.nxs;C:/dir2/file2.nxs")
        self.mock_model_to_load_workspaces([[1], [2]], [1234, 1235])
        self.model.execute = mock.Mock()

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.execute.call_count, 0)
        self.assertEqual(self.view.disable_load_buttons.call_count, 0)
        self.assertEqual(self.view.enable_load_buttons.call_count, 0)

    @run_test_with_and_without_threading
    def test_warning_shown_if_multiple_files_selected_from_browse_in_single_file_mode(self):
        self.mock_browse_button_to_return_files(["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.mock_model_to_load_workspaces([[1], [2]], [1234, 1235])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
    def test_warning_shown_if_multiple_files_entered_by_user_in_single_file_mode(self):
        self.mock_user_input_text("C:/dir1/file1.nxs;C:/dir2/file2.nxs")
        self.mock_model_to_load_workspaces([[1], [2]], [1234, 1235])

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
    def test_single_file_from_browse_loaded_into_model_and_view_in_single_file_mode(self):
        self.mock_browse_button_to_return_files(["C:/dir1/file1.nxs"])
        self.mock_model_to_load_workspaces([[1]], [1234])
        self.view.set_file_edit = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [[1]])
        self.assertEqual(self.model.loaded_runs, [1234])

        self.view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs", mock.ANY)

    @run_test_with_and_without_threading
    def test_single_file_from_user_input_loaded_into_model_and_view_in_single_file_mode(self):
        self.view.set_file_edit = mock.Mock()
        self.mock_model_to_load_workspaces([[1]], [1234])
        self.mock_user_input_text("C:/dir1/file1.nxs")

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [[1]])
        self.assertEqual(self.model.loaded_runs, [1234])

        self.view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs", mock.ANY)

    @run_test_with_and_without_threading
    def test_that_if_invalid_file_selected_in_browser_view_does_not_change(self):
        self.mock_browse_button_to_return_files(["not_a_file"])
        self.mock_model_to_load_workspaces([[1]], [1234])

        self.view.set_file_edit = mock.Mock()
        self.view.reset_edit_to_cached_value = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        set_file_edit_count = self.view.set_file_edit.call_count
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.set_file_edit.call_count, set_file_edit_count)
        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 0)

    @run_test_with_and_without_threading
    def test_that_view_reverts_to_previous_text_if_users_supplies_invalid_text(self):
        self.load_workspaces_into_model_and_view_from_browse([[1]], [1234], ["C:/dir1/EMU0001234.nxs"])

        invalid_user_input = ["some random text", "1+1=2", "..."]

        call_count = self.view.reset_edit_to_cached_value.call_count
        for invalid_text in invalid_user_input:
            call_count += 1
            self.view.get_file_edit_text = mock.Mock(return_value=invalid_text)

            self.presenter.handle_file_changed_by_user()
            self.Runner(self.presenter._load_thread)

            self.assertEqual(self.view.reset_edit_to_cached_value.call_count, call_count)

    @run_test_with_and_without_threading
    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_browse(self):
        self.load_workspaces_into_model_and_view_from_browse([[1]], [1234], ["C:/dir1/EMU0001234.nxs"])

        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/EMU0001234.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [[1]])
        self.assertEqual(self.model.loaded_runs, [1234])

        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 0)
        self.assertEqual(self.view.set_file_edit.call_args[0][0], "C:/dir1/EMU0001234.nxs")

    @run_test_with_and_without_threading
    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_user_input(self):
        self.load_workspaces_into_model_and_view_from_browse([[1]], [1234], ["C:/dir1/EMU0001234.nxs"])

        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)
        self.view.set_file_edit("C:\dir2\EMU000123.nxs")

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/EMU0001234.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [[1]])
        self.assertEqual(self.model.loaded_runs, [1234])

        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
