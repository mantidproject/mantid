import sys
import time

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter
from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel

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

class LoadFileWidgetPresenterTest(unittest.TestCase):

    class Runner:
        QT_APP = QtWidgets.QApplication([])

        def __init__(self, thread):
            if thread:
                self._thread = thread
                self._thread.finished.connect(self.finished)
                if self._thread.isRunning():
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

        self.mock_model = mock.create_autospec(BrowseFileWidgetModel, spec_set=True)
        self.mock_model.execute = mock.Mock()
        self.mock_model.exception_message_for_failed_files = mock.Mock()

        self.mock_view.disable_load_buttons = mock.Mock()
        self.mock_view.enable_load_buttons = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)

    def test_dialog_opens_when_browse_button_clicked(self):
        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.show_file_browser_and_return_selection.assert_called_once()

    def test_loading_not_initiated_if_no_file_selected_from_browser(self):
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=[])
        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.handle_load_thread_start = mock.Mock()
        presenter.on_browse_button_clicked()

        presenter.handle_load_thread_start.assert_not_called()

    def test_buttons_disabled_while_load_thread_running(self):
        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()

    def test_buttons_enabled_even_if_load_thread_throws(self):
        def load_failure():
            raise ValueError("Error text")

        self.mock_model.execute = mock.Mock(side_effect=load_failure)

        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()

    def test_files_not_loaded_into_model_if_multiple_files_selected_from_browse_in_single_file_mode(self):
        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        model.execute = mock.Mock()
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        # TODO : fix to check for warning box
        self.assertRaises(ValueError, presenter.on_browse_button_clicked)
        model.execute.assert_not_called()
        self.mock_view.disable_load_buttons.assert_not_called()
        self.mock_view.enable_load_buttons.assert_not_called()

    def test_files_not_loaded_into_model_if_multiple_files_entered_by_user_in_single_file_mode(self):
        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        model.execute = mock.Mock()
        self.mock_view.get_file_edit_text = mock.Mock(
            return_value="C:/dir1/file1.nxs;C:/dir2/file2.nxs")

        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        # TODO : fix to check for warning box
        with self.assertRaises(ValueError):
            presenter.handle_file_changed_by_user()
        model.execute.assert_not_called()
        self.mock_view.disable_load_buttons.assert_not_called()
        self.mock_view.enable_load_buttons.assert_not_called()

    def test_single_file_from_browse_loaded_into_model_in_single_file_mode(self):
        muon_file = "C:/dir1/file1.nxs"
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[muon_file])
        self.mock_view.set_file_edit = mock.Mock()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        presenter.on_browse_button_clicked()
        # presenter._load_thread.wait()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])
        self.assertEqual(model.loaded_workspaces, [muon_workspace])
        self.assertEqual(model.loaded_runs, [muon_run])

        self.mock_view.set_file_edit.assert_called_once_with(muon_file, mock.ANY)

    def test_single_file_from_user_input_loaded_into_model_in_single_file_mode(self):
        muon_file = "C:/dir1/file1.nxs"
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        # self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=[muon_file])
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.get_file_edit_text = mock.Mock(return_value=muon_file)
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        presenter.handle_file_changed_by_user()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])
        self.assertEqual(model.loaded_workspaces, [muon_workspace])
        self.assertEqual(model.loaded_runs, [muon_run])

        self.mock_view.set_file_edit.assert_called_once_with(muon_file, mock.ANY)

    def test_multiple_files_from_browse_loaded_into_model_in_multiple_file_mode(self):
        muon_files = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir3/file3.nxs"]
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_files)
        self.mock_view.set_file_edit = mock.Mock()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(True)

        presenter.on_browse_button_clicked()
        # presenter._load_thread.wait()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, muon_files)
        self.assertEqual(model.loaded_workspaces, [muon_workspace, muon_workspace, muon_workspace])
        self.assertEqual(model.loaded_runs, [muon_run, muon_run, muon_run])

        self.mock_view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs;C:/dir2/file2.nxs;C:/dir3/file3.nxs",
                                                             mock.ANY)

    def test_that_if_invalid_file_selected_in_browser_view_does_not_change(self):
        muon_file = "C:/dir1/file1.nxs"

        def load_failure():
            raise ValueError("Error text")

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=[muon_file])
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_file)
        self.mock_view.set_file_edit = mock.Mock()

        self.mock_view.reset_edit_to_cached_value = mock.Mock()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(True)

        presenter.on_browse_button_clicked()
        # presenter._load_thread.wait()
        runner = self.Runner(presenter._load_thread)
        # runner.QT_APP.exec_()

        model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)

        cc = self.mock_view.set_file_edit.call_count
        presenter.on_browse_button_clicked()
        # presenter._load_thread.wait()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(self.mock_view.set_file_edit.call_count, cc)
        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 0)

    def test_that_cannot_load_same_file_twice_from_same_browse(self):
        muon_files = ["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"]
        muon_files_unique = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"]
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_files)
        self.mock_view.set_file_edit = mock.Mock()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(True)

        presenter.on_browse_button_clicked()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, muon_files_unique)
        self.assertEqual(model.loaded_workspaces, [muon_workspace, muon_workspace])
        self.assertEqual(model.loaded_runs, [muon_run, muon_run])

        self.mock_view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs;C:/dir2/file2.nxs",
                                                             mock.ANY)

    def test_that_cannot_load_same_file_twice_from_user_input(self):
        user_input = "C:/dir1/file1.nxs;C:/dir2/file1.nxs;C:/dir2/file2.nxs"
        muon_files = ["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"]
        muon_files_unique = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"]
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234

        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(return_value=muon_files)
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.get_file_edit_text = mock.Mock(return_value=user_input)
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(True)

        presenter.handle_file_changed_by_user()
        runner = self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, muon_files_unique)
        self.assertEqual(model.loaded_workspaces, [muon_workspace, muon_workspace])
        self.assertEqual(model.loaded_runs, [muon_run, muon_run])

        self.mock_view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs;C:/dir2/file2.nxs",
                                                             mock.ANY)

    def test_that_view_reverts_to_previous_text_if_users_supplies_invalid_text(self):
        muon_workspace = [1, 2, 3, 4]
        muon_run = 1234
        model = BrowseFileWidgetModel(MuonLoadData())
        model.load_workspace_from_filename = mock.Mock(return_value=(muon_workspace, muon_run))
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.reset_edit_to_cached_value = mock.Mock()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(True)

        presenter.handle_file_changed_by_user()
        runner = self.Runner(presenter._load_thread)

        invalid_user_input = ["some random text", "1+1=2", "..."]
        call_count = self.mock_view.reset_edit_to_cached_value.call_count
        for invalid_text in invalid_user_input:
            call_count += 1
            self.mock_view.get_file_edit_text = mock.Mock(return_value=invalid_text)
            presenter.handle_file_changed_by_user()
            self.Runner(presenter._load_thread)
            self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, call_count)


    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_browse(self):
        muon_file = "C:/dir1/EMU0001234.nxs"

        def load_failure():
            raise ValueError("Error text")

        # setup initial file
        model = BrowseFileWidgetModel(MuonLoadData())
        view = BrowseFileWidgetView()

        view.show_file_browser_and_return_selection = mock.Mock(return_value = [muon_file])
        model.load_workspace_from_filename = mock.Mock(return_value=([1], 1234))

        presenter = BrowseFileWidgetPresenter(view, model)
        presenter.enable_multiple_files(False)
        presenter.on_browse_button_clicked()
        self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])


        model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)

        presenter.on_browse_button_clicked()
        self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])
        self.assertEqual(model.loaded_workspaces, [[1]])
        self.assertEqual(model.loaded_runs, [1234])

        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 0)

    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_user_input(self):
        muon_file = "C:/dir1/EMU0001234.nxs"

        def load_failure():
            raise ValueError("Error text")

        # setup initial file
        model = BrowseFileWidgetModel(MuonLoadData())
        view = BrowseFileWidgetView()

        view.show_file_browser_and_return_selection = mock.Mock(return_value = [muon_file])
        model.load_workspace_from_filename = mock.Mock(return_value=([1], 1234))

        presenter = BrowseFileWidgetPresenter(view, model)
        presenter.enable_multiple_files(False)
        presenter.on_browse_button_clicked()
        self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])


        model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)
        view.set_file_edit("C:\dir2\EMU000123.nxs")

        presenter.handle_file_changed_by_user()
        self.Runner(presenter._load_thread)

        self.assertEqual(model.loaded_filenames, [muon_file])
        self.assertEqual(model.loaded_workspaces, [[1]])
        self.assertEqual(model.loaded_runs, [1234])

        self.assertEqual(self.mock_view.reset_edit_to_cached_value.call_count, 0)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
