import unittest

import six
from mantid.py3compat import mock

from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


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


class LoadFileWidgetPresenterMultipleFileModeTest(unittest.TestCase):
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
            self._qapp.processEvents()

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        setup_context_for_tests(self)
        self.data_context.instrument = 'EMU'
        self.view = BrowseFileWidgetView()
        self.model = BrowseFileWidgetModel(self.loaded_data, self.context)

        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()
        self.view.warning_popup = mock.Mock()

        self.presenter = BrowseFileWidgetPresenter(self.view, self.model)

        patcher = mock.patch('Muon.GUI.Common.load_file_widget.model.load_utils.load_workspace_from_filename')
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.return_value = (self.create_fake_workspace(1), '22222', 'filename')

    def mock_loading_multiple_files_from_browse(self, runs, workspaces, filenames):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=filenames)
        self.load_utils_patcher.side_effect = zip(workspaces, runs, filenames)

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = 'EMU'
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {'OutputWorkspace': [workspace_mock]}

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Multiple runs can be selected via browse and entered explicitly using the ";" separator
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_that_cannot_load_same_file_twice_from_same_browse_even_if_filepaths_are_different(self):
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.call_count, 2)
        self.load_utils_patcher.assert_any_call("C:/dir1/file1.nxs")
        self.load_utils_patcher.assert_any_call("C:/dir2/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_cannot_load_same_file_twice_from_user_input_even_if_filepaths_are_different(self):
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
        self.load_utils_patcher.side_effect = zip([workspace_1, workspace_2],
                                                  [1234, 1234],
                                                  ["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"])
        self.view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        # Load will take the last occurrence of the run from the list
        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file2.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [workspace_2])
        six.assertCountEqual(self, self.model.loaded_runs, [[1234]])

    #
    @run_test_with_and_without_threading
    def test_that_cannot_input_and_load_same_run_twice_even_if_filenames_are_different(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip([workspace_1, workspace_2], [1234, 1234],
                                                  ["C:/dir1/file1.nxs", "C:/dir1/file2.nxs"])
        self.view.set_file_edit("C:/dir1/file1.nxs;C:/dir1/file2.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        # Load will take the last occurrence of the run from the user input
        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir1/file2.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [workspace_2])
        six.assertCountEqual(self, self.model.loaded_runs, [[1234]])

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_browse_sets_model_and_interface_correctly(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

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
        self.load_utils_patcher.side_effect = zip([workspace_1, workspace_2],
                                                  [1234, 1235],
                                                  ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])
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
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2],
                                                     ["C:/dir1/file2.nxs", "C:/dir1/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_two_files_from_user_input_sets_interface_alphabetically(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        self.load_utils_patcher.side_effect = zip([workspace_2, workspace_1], [1235, 1234],
                                                  ["C:/dir1/file2.nxs", "C:/dir1/file1.nxs"])
        self.view.set_file_edit("C:/dir1/file2.nxs;C:/dir1/file1.nxs")

        self.presenter.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir1/file2.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_multiple_files_from_browse_ignores_loads_which_throw(self):
        workspace = self.create_fake_workspace(1)

        files = ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir2/file3.nxs"]
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value=files)
        load_return_values = [(workspace, 1234 + i, filename) for i, filename in enumerate(files)]
        self.load_utils_patcher.side_effect = iter(IteratorWithException(load_return_values, [1]))

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.model.loaded_filenames, ["C:/dir1/file1.nxs", "C:/dir2/file3.nxs"])
        self.assertEqual(self.model.loaded_runs, [[1234], [1236]])
        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file1.nxs;C:/dir2/file3.nxs")

    @run_test_with_and_without_threading
    def test_that_browse_allows_loading_of_additional_files(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        workspace_3 = self.create_fake_workspace(3)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        self.mock_loading_multiple_files_from_browse([1236], [workspace_3], ["C:/dir1/file3.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames,
                             ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs", "C:/dir1/file3.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [workspace_1, workspace_2, workspace_3])
        six.assertCountEqual(self, self.model.loaded_runs, [[1234], [1235], [1236]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir1/file3.nxs")

    @run_test_with_and_without_threading
    def test_that_loading_an_already_loaded_run_from_different_file_overwrites(self):
        workspace_1 = self.create_fake_workspace(1)
        workspace_2 = self.create_fake_workspace(2)
        workspace_3 = self.create_fake_workspace(3)
        self.mock_loading_multiple_files_from_browse([1234, 1235], [workspace_1, workspace_2],
                                                     ["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        # only checks runs, so can have a different file/workspace (this is why overwriting is
        # the most useful behaviour in this situation).
        self.mock_loading_multiple_files_from_browse([1234], [workspace_3], ["C:/dir2/file1.nxs"])

        self.presenter.on_browse_button_clicked()
        self.wait_for_thread(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["C:/dir2/file1.nxs", "C:/dir2/file2.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [workspace_3, workspace_2])
        six.assertCountEqual(self, self.model.loaded_runs, [[1234], [1235]])

        self.assertEqual(self.view.get_file_edit_text(), "C:/dir2/file1.nxs")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
