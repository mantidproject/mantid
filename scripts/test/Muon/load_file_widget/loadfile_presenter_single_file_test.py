# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from qtpy.QtWidgets import QApplication

from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


class LoadFileWidgetPresenterTest(GuiTest):
    def run_test_with_and_without_threading(test_function):

        def run_twice(self):
            test_function(self)
            self.setUp()
            self.presenter._use_threading = False
            test_function(self)

        return run_twice

    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            QApplication.instance().processEvents()

    def setUp(self):
        self.view = BrowseFileWidgetView()

        self.view.on_browse_clicked = mock.Mock()
        self.view.set_file_edit = mock.Mock()
        self.view.reset_edit_to_cached_value = mock.Mock()
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        setup_context_for_tests(self)

        self.data_context.instrument = 'EMU'
        self.model = BrowseFileWidgetModel(self.loaded_data, self.context)
        self.model.exception_message_for_failed_files = mock.Mock()

        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()
        self.view.warning_popup = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.view, self.model)

        patcher = mock.patch('Muon.GUI.Common.load_file_widget.model.load_utils.load_workspace_from_filename')
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.return_value = (self.create_fake_workspace(1), '22222', 'filename')

    def mock_browse_button_to_return_files(self, files):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=files)

    def mock_user_input_text(self, text):
        self.view.get_file_edit_text = mock.Mock(return_value=text)

    def mock_model_to_load_workspaces(self, workspaces, runs, filenames):
        self.load_utils_patcher.side_effect = zip(workspaces, runs, filenames)

    def load_workspaces_into_model_and_view_from_browse(self, workspaces, runs, files):
        self.mock_model_to_load_workspaces(workspaces, runs, files)
        self.mock_browse_button_to_return_files(files)

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

    def load_failure(self, unused_arg):
        raise ValueError("Error text")

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = 'EMU'
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {'OutputWorkspace': [workspace_mock]}

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_browser_dialog_opens_when_browse_button_clicked(self):
        self.mock_browse_button_to_return_files(["file.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.show_file_browser_and_return_selection.call_count, 1)

    @run_test_with_and_without_threading
    def test_loading_not_initiated_if_no_file_selected_from_browser(self):
        self.mock_model_to_load_workspaces([], [], [])
        self.mock_browse_button_to_return_files([])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.call_count, 0)

    @run_test_with_and_without_threading
    def test_buttons_disabled_while_load_thread_running(self):
        self.mock_browse_button_to_return_files(["file.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.load_utils_patcher.assert_called_once_with("file.nxs")
        self.assertEqual(self.view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.view.enable_load_buttons.call_count, 1)

    @run_test_with_and_without_threading
    def test_buttons_enabled_after_load_even_if_load_thread_throws(self):
        self.mock_browse_button_to_return_files(["file.nxs"])
        self.load_utils_patcher.side_effect = self.load_failure

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.load_utils_patcher.assert_called_once_with("file.nxs")
        self.assertEqual(self.view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.view.enable_load_buttons.call_count, 1)

    @run_test_with_and_without_threading
    def test_single_file_from_browse_loaded_into_model_and_view_in_single_file_mode(self):
        workspace = self.create_fake_workspace(1)

        self.mock_browse_button_to_return_files(["C:/dir1/file1.nxs"])
        self.mock_model_to_load_workspaces([workspace], [1234], ["C:/dir1/file1.nxs"])
        self.view.set_file_edit = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace])
        self.assertEqual(self.model.loaded_runs, [[1234]])

        self.view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs", mock.ANY)

    @run_test_with_and_without_threading
    def test_single_file_from_user_input_loaded_into_model_and_view_in_single_file_mode(self):
        workspace = self.create_fake_workspace(1)

        self.view.set_file_edit = mock.Mock()
        self.mock_model_to_load_workspaces([workspace], [1234], ["C:/dir1/file1.nxs"])
        self.mock_user_input_text("C:/dir1/file1.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace])
        self.assertEqual(self.model.loaded_runs, [[1234]])

        self.view.set_file_edit.assert_called_once_with("C:/dir1/file1.nxs", mock.ANY)

    @run_test_with_and_without_threading
    def test_that_if_invalid_file_selected_in_browser_view_does_not_change(self):
        workspace = self.create_fake_workspace(1)

        self.mock_browse_button_to_return_files(["not_a_file"])
        self.mock_model_to_load_workspaces([workspace], [1234], ["not_a_file"])

        self.view.set_file_edit = mock.Mock()
        self.view.reset_edit_to_cached_value = mock.Mock()

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.load_utils_patcher.side_effect = self.load_failure

        set_file_edit_count = self.view.set_file_edit.call_count
        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.set_file_edit.call_count, set_file_edit_count)
        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 0)

    @run_test_with_and_without_threading
    def test_that_view_reverts_to_previous_text_if_users_supplies_invalid_text(self):
        workspace = self.create_fake_workspace(1)

        self.load_workspaces_into_model_and_view_from_browse([workspace], [[1234]], ["C:/dir1/EMU0001234.nxs"])

        invalid_user_input = ["some random text", "1+1=2", "..."]

        call_count = self.view.reset_edit_to_cached_value.call_count
        for invalid_text in invalid_user_input:
            call_count += 1
            self.view.get_file_edit_text = mock.Mock(return_value=invalid_text)

            self.presenter.handle_file_changed_by_user()
            self.wait_for_thread(self.presenter._load_thread)

            self.assertEqual(self.view.reset_edit_to_cached_value.call_count, call_count)

    @run_test_with_and_without_threading
    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_browse(self):
        workspace = self.create_fake_workspace(1)
        self.load_workspaces_into_model_and_view_from_browse([workspace], [1234], ["C:/dir1/EMU0001234.nxs"])

        self.load_utils_patcher.side_effect = self.load_failure

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/EMU0001234.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace])
        self.assertEqual(self.model.loaded_runs, [[1234]])

        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 0)
        self.assertEqual(self.view.set_file_edit.call_args[0][0], "C:/dir1/EMU0001234.nxs")

    @run_test_with_and_without_threading
    def test_that_model_and_interface_revert_to_previous_values_if_load_fails_from_user_input(self):
        workspace = self.create_fake_workspace(1)
        self.load_workspaces_into_model_and_view_from_browse([workspace], [1234], ["C:/dir1/EMU0001234.nxs"])

        self.load_utils_patcher.side_effect = self.load_failure
        self.view.set_file_edit("C:\dir2\EMU000123.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/EMU0001234.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace])
        self.assertEqual(self.model.loaded_runs, [[1234]])

        self.assertEqual(self.view.reset_edit_to_cached_value.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
