# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import time

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication
from mantidqtinterfaces.Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


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


@start_qapplication
class LoadFileWidgetPresenterMultipleFileModeTest(unittest.TestCase):
    def run_test_with_and_without_threading(test_function):
        def run_twice(self):
            test_function(self)
            self.setUp()
            self.presenter._use_threading = False
            test_function(self)

        return run_twice

    def wait_for_thread(self, thread_model):
        if thread_model and thread_model.worker:
            while thread_model.worker.is_alive():
                time.sleep(0.1)
            QApplication.sendPostedEvents()

    def setUp(self):
        setup_context_for_tests(self)
        self.data_context.instrument = "EMU"
        self.view = BrowseFileWidgetView()
        self.model = BrowseFileWidgetModel(self.loaded_data, self.context)

        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()
        self.view.warning_popup = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.view, self.model)

        patcher = mock.patch("mantidqtinterfaces.Muon.GUI.Common.load_file_widget.model.load_utils.load_workspace_from_filename")
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.return_value = (self.create_fake_workspace(1), "22222", "filename")

    def mock_loading_multiple_files_from_browse(self, runs, workspaces, filenames):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=filenames)
        psi_data = [False] * len(filenames)
        self.load_utils_patcher.side_effect = zip(workspaces, runs, filenames, psi_data)

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = "EMU"
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {"OutputWorkspace": [workspace_mock]}

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Multiple runs can be selected via browse and entered explicitly using the ";" separator
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_that_cannot_load_same_file_twice_from_same_browse_even_if_filepaths_are_different(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"]
        )
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            side_effect=zip([workspace_1, workspace_2], [1234, 1234], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"], [False, False])
        )

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.call_count, 2)
        self.load_utils_patcher.assert_any_call("C:/dir1/file1.nxs")
        self.load_utils_patcher.assert_any_call("C:/dir2/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_cannot_load_same_file_twice_from_user_input_even_if_filepaths_are_different(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            side_effect=zip([workspace_1, workspace_2], [1234, 1234], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"], [False, False])
        )
        self.view.set_file_edit("C:/dir1/file1.nxs;C:/dir2/file1.nxs;C:/dir2/file2.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.call_count, 2)
        self.load_utils_patcher.assert_any_call("C:/dir1/file1.nxs")
        self.load_utils_patcher.assert_any_call("C:/dir2/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_cannot_browse_and_load_same_run_twice_even_if_filenames_are_different(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip(
            [workspace_1, workspace_2], [1234, 1234], ["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"], [False, False]
        )
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        # Load will take the last occurrence of the run from the list
        self.assertCountEqual(self.model.loaded_filenames, ["C:/dir1/file2.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [workspace_2])
        self.assertCountEqual(self.model.loaded_runs, [[1234]])

    #
    @run_test_with_and_without_threading
    def test_that_cannot_input_and_load_same_run_twice_even_if_filenames_are_different(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip(
            [workspace_1, workspace_2], [1234, 1234], ["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"], [False, False]
        )
        self.view.set_file_edit("C:/dir1/file1.nxs;C:/dir1/file2.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        # Load will take the last occurrence of the run from the user input
        self.assertCountEqual(self.model.loaded_filenames, ["C:/dir1/file2.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [workspace_2])
        self.assertCountEqual(self.model.loaded_runs, [[1234]])

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_browse_sets_model_and_interface_correctly(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace_1, workspace_2])
        self.assertEqual(self.model.loaded_runs, [[1234], [1235]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_user_input_sets_model_and_interface_correctly(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip(
            [workspace_1, workspace_2], [1234, 1235], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"], [False, False]
        )
        self.view.set_file_edit("C:/dir1/file1.nxs;C:/dir2/file2.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
        self.assertEqual(self.model.loaded_workspaces, [workspace_1, workspace_2])
        self.assertEqual(self.model.loaded_runs, [[1234], [1235]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_browse_sets_interface_alphabetically(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2], ["C:/dir1/file2.nxs", "C:/dir1/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_user_input_sets_interface_alphabetically(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip(
            [workspace_2, workspace_1], [1235, 1234], ["C:/dir1/file2.nxs", "C:/dir1/file1.nxs"], [False, False]
        )
        self.view.set_file_edit("C:/dir1/file2.nxs;C:/dir1/file1.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_browse_allows_loading_of_additional_files(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        workspace_3 = self.create_fake_workspace(3)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.mock_loading_multiple_files_from_browse([1236], [workspace_3], ["C:/dir1/file3.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir1/file3.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [workspace_1, workspace_2, workspace_3])
        self.assertCountEqual(self.model.loaded_runs, [[1234], [1235], [1236]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file3.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_an_already_loaded_run_from_different_file_overwrites(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        workspace_3 = self.create_fake_workspace(3)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2], ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        # only checks runs, so can have a different file/workspace (this is why overwriting is
        # the most useful behaviour in this situation).
        self.mock_loading_multiple_files_from_browse([1234], [workspace_3], ["C:/dir2/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["C:/dir2/file1.nxs", "C:/dir2/file2.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [workspace_3, workspace_2])
        self.assertCountEqual(self.model.loaded_runs, [[1234], [1235]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir2/file1.nxs")


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
