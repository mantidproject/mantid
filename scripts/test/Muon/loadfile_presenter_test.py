import sys
import time

import unittest
import six

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter
from Muon.GUI.MuonAnalysis.loadfile.load_file_model_multithreading import BrowseFileWidgetModel

from Muon.GUI.Common.muon_load_data import MuonLoadData

from qtpy import QtWidgets


def wait_for_thread(thread, timeout=10):
    print("waiting")
    start = time.time()
    while (time.time() - start < timeout):
        time.sleep(0.1)
        if thread.isFinished():
            return True
    return False


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

class LoadFileWidgetPresenterTest(unittest.TestCase):
    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                if self._thread.is_running():
                    self.QT_APP.exec_()

        def finished(self):
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        self.mock_view = mock.create_autospec(BrowseFileWidgetView, spec_set=True)

        # self.mock_view = BrowseFileWidgetView()
        self.mock_view.on_browse_clicked = mock.Mock()
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.mock_model = BrowseFileWidgetModel(MuonLoadData())
        # self.mock_model.execute = mock.Mock()
        self.mock_model.exception_message_for_failed_files = mock.Mock()

        self.mock_view.disable_load_buttons = mock.Mock()
        self.mock_view.enable_load_buttons = mock.Mock()
        self.mock_view.warning_popup = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)

    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Multiple runs can be selected via browse and entered explicitly using the ";" separator
    # ------------------------------------------------------------------------------------------------------------------

    def test_dialog_opens_when_browse_button_clicked(self):
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.show_file_browser_and_return_selection.call_count,1)

    def test_loading_not_initiated_if_no_file_selected_from_browser(self):
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[])
        self.presenter.handle_load_thread_start = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.presenter.handle_load_thread_start.assert_not_called()

    def test_buttons_disabled_while_load_thread_running(self):
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.disable_load_buttons.call_count,1)
        self.assertEqual(self.mock_view.enable_load_buttons.call_count,1)

    def test_buttons_enabled_even_if_load_throws(self):
        self.mock_model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.disable_load_buttons.call_count,1)
        self.assertEqual(self.mock_view.enable_load_buttons.call_count,1)

    def test_files_not_loaded_into_model_if_multiple_files_selected_from_browse_in_single_file_mode(self):
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=("file.nxs", [1, 2, 3, 4], 1234))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.presenter.enable_multiple_files(False)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.mock_model.load_workspace_from_filename.assert_not_called()
        self.mock_view.disable_load_buttons.assert_not_called()
        self.mock_view.enable_load_buttons.assert_not_called()

    def test_warning_shown_if_multiple_files_selected_from_browse_in_single_file_mode(self):
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=("file.nxs", [1, 2, 3, 4], 1234))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.presenter.enable_multiple_files(False)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.warning_popup.call_count, 1)

    def test_files_not_loaded_into_model_if_multiple_files_entered_by_user_in_single_file_mode(self):
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        self.mock_view.get_file_edit_text = mock.Mock(return_value="C:/dir1/file1.nxs;C:/dir2/file2.nxs")
        self.presenter.enable_multiple_files(False)

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        self.mock_model.load_workspace_from_filename.assert_not_called()
        self.mock_view.disable_load_buttons.assert_not_called()
        self.mock_view.enable_load_buttons.assert_not_called()

    def test_warning_shown_if_multiple_files_entered_by_user_in_single_file_mode(self):
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        self.mock_view.get_file_edit_text = mock.Mock(return_value="C:/dir1/file1.nxs;C:/dir2/file2.nxs")
        self.presenter.enable_multiple_files(False)

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.warning_popup.call_count, 1)

    def test_single_file_from_browse_loaded_into_model_in_single_file_mode(self):
        muon_file = "C:/dir1/file1.nxs"
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=(muon_file, muon_workspace, muon_run))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[muon_file])
        self.mock_view.set_file_edit = mock.Mock()
        self.presenter.enable_multiple_files(False)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])
        self.assertEqual(self.mock_model.loaded_workspaces, [muon_workspace])
        self.assertEqual(self.mock_model.loaded_runs, [muon_run])

        self.mock_view.set_file_edit.assert_called_once_with(muon_file, mock.ANY)

    def test_single_file_from_user_input_loaded_into_model_in_single_file_mode(self):
        muon_file = "C:/dir1/file1.nxs"
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=(muon_file, muon_workspace, muon_run))
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.get_file_edit_text = mock.Mock(return_value=muon_file)
        self.presenter.enable_multiple_files(False)

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])
        self.assertEqual(self.mock_model.loaded_workspaces, [muon_workspace])
        self.assertEqual(self.mock_model.loaded_runs, [muon_run])

        self.mock_view.set_file_edit.assert_called_once_with(muon_file, mock.ANY)

    def test_that_if_invalid_file_selected_in_browser_view_does_not_change(self):
        muon_file = "C:/dir1/file1.nxs"

        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=[muon_file])
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_file)
        self.mock_view.set_file_edit = mock.Mock()

        self.mock_view.reset_edit_to_cached_value = mock.Mock()
        self.presenter.enable_multiple_files(True)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.mock_model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        cc = self.mock_view.set_file_edit.call_count
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_view.set_file_edit.call_count, cc)
        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 0)

    def test_that_cannot_load_same_file_twice_from_same_browse(self):
        muon_files = ["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"]
        muon_files_unique = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"]
        muon_workspace = [[1, 2, 3, 4], [1, 2, 3, 4]]
        muon_run = [1234, 1235]

        self.mock_model.load_workspace_from_filename = mock.Mock(
            side_effect=iter(zip(muon_files_unique, muon_workspace, muon_run)))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_files)
        self.mock_view.set_file_edit = mock.Mock()
        self.presenter.enable_multiple_files(True)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.mock_model.loaded_filenames, muon_files_unique)
        six.assertCountEqual(self, self.mock_model.loaded_workspaces, muon_workspace)
        six.assertCountEqual(self, self.mock_model.loaded_runs, muon_run)

        self.mock_view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs;C:/dir2/file2.nxs",
                                                             mock.ANY)

    def test_that_cannot_load_same_file_twice_from_user_input(self):
        user_input = "C:/dir1/file1.nxs;C:/dir2/file1.nxs;C:/dir2/file2.nxs"
        muon_files = ["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"]
        muon_files_unique = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"]
        muon_workspace = [[1], [2]]
        muon_run = [1234, 1235]

        self.mock_model.load_workspace_from_filename = mock.Mock(
            side_effect=iter(zip(muon_files_unique, muon_workspace, muon_run)))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_files)
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.get_file_edit_text = mock.Mock(return_value=user_input)
        self.presenter.enable_multiple_files(True)

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.mock_model.loaded_filenames, muon_files_unique)
        six.assertCountEqual(self, self.mock_model.loaded_workspaces, muon_workspace)
        six.assertCountEqual(self, self.mock_model.loaded_runs, muon_run)

        self.mock_view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs;C:/dir2/file2.nxs",
                                                             mock.ANY)

    def test_that_view_reverts_to_previous_text_if_users_supplies_invalid_text(self):
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=("file.nxs", muon_workspace, muon_run))
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.reset_edit_to_cached_value = mock.Mock()
        self.presenter.enable_multiple_files(True)

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        invalid_user_input = ["some random text", "1+1=2", "..."]
        call_count = self.mock_view.reset_edit_to_cached_value.call_count
        for invalid_text in invalid_user_input:
            call_count += 1
            self.mock_view.get_file_edit_text = mock.Mock(return_value=invalid_text)
            self.presenter.handle_file_changed_by_user()
            self.Runner(self.presenter._model.thread_manager)
            self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, call_count)

    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_browse(self):
        muon_file = "C:/dir1/EMU0001234.nxs"

        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[muon_file])
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=(muon_file, [1], 1234))

        self.presenter.enable_multiple_files(False)
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])

        self.mock_model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])
        self.assertEqual(self.mock_model.loaded_workspaces, [[1]])
        self.assertEqual(self.mock_model.loaded_runs, [1234])

        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 0)

    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_user_input(self):
        muon_file = "C:/dir1/EMU0001234.nxs"

        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[muon_file])
        self.mock_model.load_workspace_from_filename = mock.Mock(return_value=(muon_file, [1], 1234))

        self.presenter.enable_multiple_files(False)
        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])

        self.mock_model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)
        self.mock_view.set_file_edit("C:\dir2\EMU000123.nxs")

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.mock_model.loaded_filenames, [muon_file])
        self.assertEqual(self.mock_model.loaded_workspaces, [[1]])
        self.assertEqual(self.mock_model.loaded_runs, [1234])

        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 1)


class LoadFileWidgetPresenterMultipleFileModeTest(unittest.TestCase):
    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                # if self._thread.is_running():
                print("### THREAD RUNNING ", time.time())
                self.QT_APP.exec_()

        def finished(self):
            print("#### FINISHED ", time.time())
            self.QT_APP.processEvents()
            self.QT_APP.exit(0)

    def setUp(self):
        self.data = MuonLoadData()
        self.view = BrowseFileWidgetView()
        self.model = BrowseFileWidgetModel(self.data)

        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.view, self.model)
        self.presenter.enable_multiple_files(True)

    def mock_loading_multiple_files_from_browse(self, runs, workspaces, filenames):
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=filenames)
        self.model.load_workspace_from_filename = mock.Mock(side_effect=zip(filenames, workspaces, runs))

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Multiple runs can be selected via browse and entered explicitly using the ";" separator
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_loading_two_files_from_browse_sets_model_and_interface_correctly(self):
        self.mock_loading_multiple_files_from_browse([1234, 1235], [[1], [2]],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2]])
        six.assertCountEqual(self, self.model.loaded_runs, [1234, 1235])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file2.nxs")

    def test_that_loading_two_files_from_user_input_sets_model_and_interface_correctly(self):
        self.model.load_workspace_from_filename = mock.Mock(
            side_effect=zip(["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"], [[1], [2]], [1234, 1235]))
        self.view.set_file_edit("C:/dir1/file1.nxs;C:/dir2/file2.nxs")

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2]])
        six.assertCountEqual(self, self.model.loaded_runs, [1234, 1235])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file2.nxs")

    def test_that_loading_two_files_from_browse_sets_interface_alphabetically(self):
        self.mock_loading_multiple_files_from_browse([1234, 1235], [[1], [2]],
                                                     ["C:/dir1/file2.nxs", "C:/dir1/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    def test_that_loading_two_files_from_user_input_sets_interface_alphabetically(self):
        self.model.load_workspace_from_filename = mock.Mock(
            side_effect=zip(["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"], [[1], [2]], [1234, 1235]))
        self.view.set_file_edit("C:/dir1/file2.nxs;C:/dir1/file1.nxs")

        self.presenter.handle_file_changed_by_user()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    def test_that_loading_multiple_files_from_browse_ignores_loads_which_throw(self):
        files = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir2/file3.nxs"]
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=files)
        load_return_values = [(filename, [1], 1234 + i) for i, filename in enumerate(files)]
        self.model.load_workspace_from_filename = mock.Mock(
            side_effect=iter(IteratorWithException(load_return_values, [1])))

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.model.loaded_runs, [1234, 1236])
        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file3.nxs"])
        six.assertCountEqual(self, self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file3.nxs")

    def test_that_browse_allows_loading_of_additional_files(self):
        self.mock_loading_multiple_files_from_browse([1234, 1235], [[1], [2]],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.mock_loading_multiple_files_from_browse([1236], [[3]], ["C:/dir1/file3.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        print("Got to here!")

        six.assertCountEqual(self, self.model.loaded_filenames,
                             ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir1/file3.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2], [3]])
        six.assertCountEqual(self, self.model.loaded_runs, [1234, 1235, 1236])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file3.nxs;C:/dir2/file2.nxs")

    def test_that_loading_an_already_loaded_file_overwrites_it(self):
        self.mock_loading_multiple_files_from_browse([1234, 1235], [[1], [2]],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        # only checks runs, so can have a different file/workspace (this is why overwriting is
        # the most useful behaviour in this situation).
        self.mock_loading_multiple_files_from_browse([1235], [[3]], ["C:/dir2/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file1.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [3]])
        six.assertCountEqual(self, self.model.loaded_runs, [1234, 1235])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file1.nxs")

    def test_that_progress_bar_appears_and_then_disappears_when_loading_more_than_10_files(self):
        runs = [1234 + i for i in range(15)]
        ws = [[i] for i in range(15)]
        filenames = ["C:/dir1/file" + str(i) + ".nxs" for i in range(15)]
        self.mock_loading_multiple_files_from_browse(runs, ws, filenames)
        self.view.show_progress_bar = mock.Mock()
        self.view.remove_progress_bar = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        self.assertEqual(self.view.show_progress_bar.call_count, 1)
        self.assertEqual(self.view.remove_progress_bar.call_count, 1)

    def test_that_progress_bar_is_given_ascending_numbers_from_0_to_100(self):
        runs = [1234 + i for i in range(15)]
        ws = [[i] for i in range(15)]
        filenames = ["C:/dir1/file" + str(i) + ".nxs" for i in range(15)]
        self.mock_loading_multiple_files_from_browse(runs, ws, filenames)
        self.view.show_progress_bar = mock.Mock()
        self.view.remove_progress_bar = mock.Mock()
        self.view.set_progress_bar = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.Runner(self.presenter._model.thread_manager)

        progress_args = [args[0][0] for args in self.view.set_progress_bar.call_args_list]

        self.assertEqual(progress_args[0], 0.0)
        self.assertEqual(progress_args[-1], 100.0)
        # check ascending
        self.assertEqual(sorted(progress_args), progress_args)

    def test_that_cancel_button_causes_progress_widget_to_be_removed(self):
        pass

    def test_that_cancel_button_returns_data_to_previous_value(self):
        pass

    # TODO : Check that thread manager returns to cleared state after cancel button.


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
