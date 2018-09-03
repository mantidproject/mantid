import sys
import os
import six

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.muon_file_utils as fileUtils

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from PyQt4.QtGui import QApplication

# global QApplication (get errors if > 1 instance in the code)
QT_APP = QApplication([])


class LoadRunWidgetPresenterTest(unittest.TestCase):
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
        self.data = MuonLoadData()
        self.view = LoadRunWidgetView()
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)
        self.presenter.enable_multiple_files(False)

    def mock_loading_via_user_input_run(self, workspace, filename, run):
        self.model.load_workspace_from_filename = mock.Mock(
            return_value=(workspace, filename, run))
        self.view.set_run_edit_text("1234")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_and_view_initialized_to_contain_no_data(self):
        self.assertEqual(self.presenter.filenames, [])
        self.assertEqual(self.presenter.runs, [])
        self.assertEqual(self.presenter.workspaces, [])

        self.assertEqual(self.view.get_run_edit_text(), "")

    def test_user_can_enter_a_run_and_load_it_in_single_file_mode(self):
        self.mock_loading_via_user_input_run([1, 2, 3], "EMU00001234.nxs", 1234)

        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["EMU00001234.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1, 2, 3]])

    def test_warning_message_displayed_if_user_enters_multiple_files_in_single_file_mode(self):
        self.presenter.enable_multiple_files(False)
        self.view.warning_popup = mock.Mock()
        self.view.set_run_edit_text("1234,1235,1236")

        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_data_reverts_to_previous_entry_if_user_enters_multiple_files_in_single_file_mode(self):
        self.presenter.enable_multiple_files(False)

        # Load some data
        self.mock_loading_via_user_input_run([1], "1234.nxs", 1234)
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.view.warning_popup = mock.Mock()
        self.view.set_run_edit_text("1234,1235,1236")

        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["1234.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1]])


class LoadRunWidgetIncrementDecrementSingleFileModeTest(unittest.TestCase):
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
        self.data = MuonLoadData()
        self.view = LoadRunWidgetView()
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.presenter.enable_multiple_files(False)

        self.load_single_run()

    def load_single_run(self):
        self._loaded_run = 1234
        self._loaded_filename = "EMU00001234.nxs"
        self._loaded_workspace = [1, 2, 3]

        self.model.load_workspace_from_filename = mock.Mock(
            return_value=(self._loaded_workspace, self._loaded_filename, self._loaded_run))
        self.view.set_run_edit_text(str(self._loaded_run))
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

    def assert_model_has_not_changed(self):
        self.assertEqual(self.model.loaded_workspaces, [self._loaded_workspace])
        self.assertEqual(self.model.loaded_runs, [self._loaded_run])
        self.assertEqual(self.model.loaded_filenames, [self._loaded_filename])

    def assert_view_has_not_changed(self):
        self.assertEqual(self.view.get_run_edit_text(), str(self._loaded_run))

    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement buttons in single file mode (can only load one run at a time)
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_decrement_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001233.nxs"
        load_call_count = self.model.load_workspace_from_filename.call_count

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.model.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    def test_that_increment_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001235.nxs"
        load_call_count = self.model.load_workspace_from_filename.call_count

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.model.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    def test_that_decrement_run_loads_the_data_correctly_by_overwriting_previous_run(self):
        new_run = self._loaded_run - 1
        new_filename = "EMU00001233.nxs"
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], new_filename, new_run))

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [new_filename])
        self.assertEqual(self.presenter.runs, [new_run])
        self.assertEqual(self.presenter.workspaces, [[1]])

        self.assertEqual(self.view.get_run_edit_text(), str(new_run))

    def test_that_increment_run_loads_the_data_correctly_by_overwriting_previous_run(self):
        new_run = self._loaded_run + 1
        new_filename = "EMU00001235.nxs"
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], new_filename, new_run))

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [new_filename])
        self.assertEqual(self.presenter.runs, [new_run])
        self.assertEqual(self.presenter.workspaces, [[1]])

        self.assertEqual(self.view.get_run_edit_text(), str(new_run))

    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assert_view_has_not_changed()

    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assert_view_has_not_changed()

    @mock.patch("Muon.GUI.Common.message_box.warning")
    def test_that_if_decrement_run_fails_warning_message_is_displayed(self, mock_warning):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(mock_warning.call_count, 1)

    @mock.patch("Muon.GUI.Common.message_box.warning")
    def test_that_if_increment_run_fails_warning_message_is_displayed(self, mock_warning):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(mock_warning.call_count, 1)


class LoadRunWidgetIncrementDecrementMultipleFileModeTest(unittest.TestCase):
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
        self.data = MuonLoadData()
        self.view = LoadRunWidgetView()
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

        self.presenter.enable_multiple_files(True)

    def load_runs(self, runs, filenames, workspaces):
        self.model.load_workspace_from_filename = mock.Mock(
            side_effect=iter(zip(workspaces, filenames, runs)))
        run_string = ",".join([str(run) for run in runs])
        self.view.set_run_edit_text(run_string)
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

    def assert_model_empty(self):
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_runs, [])

    def assert_view_empty(self):
        self.assertEqual(self.view.get_run_edit_text(), "")

    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement run buttons in "multiple file" mode
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_providing_no_runs_leaves_model_and_view_empty(self):
        self.view.set_run_edit_text("")
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    def test_that_increment_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    def test_that_decrement_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    def test_that_decrement_run_decrements_the_upper_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], "file1.nxs", 1))

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file1.nxs", "file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [1, 2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "1-4")

    def test_that_increment_run_increments_the_lower_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(return_value=([5], "file5.nxs", 5))

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs", "file5.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4], [5]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4, 5])

        self.assertEqual(self.view.get_run_edit_text(), "2-5")

    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    @mock.patch("Muon.GUI.Common.message_box.warning")
    def test_that_if_increment_run_fails_warning_message_is_displayed(self, mock_warning):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(mock_warning.call_count, 1)

    @mock.patch("Muon.GUI.Common.message_box.warning")
    def test_that_if_decrement_run_fails_warning_message_is_displayed(self, mock_warning):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(mock_warning.call_count, 1)


class LoadRunWidgetLoadCurrentRunTest(unittest.TestCase):
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

    def load_failure(self):
        raise ValueError("Error text")

    def setUp(self):
        self.view = LoadRunWidgetView()
        self.model = LoadRunWidgetModel()
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3], "currentRun.nxs", 1234))
        self.view.warning_popup = mock.Mock()
        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()

        fileUtils.get_current_run_filename = mock.Mock(return_value="EMU0001234.nxs")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_load_current_run_loads_run_into_model(self):
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["currentRun.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1, 2, 3]])

        self.assertEqual(self.model.current_run, 1234)

    def test_load_current_run_correctly_displays_run_if_load_successful(self):
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)
        self.assertEqual(self.view.get_run_edit_text(), "1234")

    @mock.patch("Muon.GUI.Common.message_box.warning")
    def test_load_current_run_displays_error_message_if_fails_to_load(self, mock_warning):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(mock_warning.call_count, 1)

    def test_load_current_run_reverts_to_previous_data_if_fails_to_load(self):
        # set up previous data
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], "1234.nxs", 1234))
        self.view.set_run_edit_text("1234")
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["1234.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1]])

    def test_load_current_run_clears_previous_data_if_load_succeeds(self):
        # set up previous data
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], "1234.nxs", 1234))
        self.view.set_run_edit_text("1234")
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.model.load_workspace_from_filename = mock.Mock(return_value=([2], "9999.nxs", 9999))
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.get_run_edit_text(), "9999")
        self.assertEqual(self.presenter.filenames, ["9999.nxs"])
        self.assertEqual(self.presenter.runs, [9999])
        self.assertEqual(self.presenter.workspaces, [[2]])

    def test_load_current_run_displays_error_if_incrementing_past_current_run(self):
        # set up current run
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], "1234.nxs", 1234))
        self.view.set_run_edit_text("1234")
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
