import sys
import os

from Muon.GUI.Common.load_run_widget.model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from PyQt4 import QtGui
from PyQt4.QtGui import QApplication

# global QApplication (get errors if > 1 instance in the code)
QT_APP = QApplication([])


class LoadRunWidgetPresenterTest(unittest.TestCase):
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
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonLoadData()
        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)
        self.presenter.enable_multiple_files(False)

    def tearDown(self):
        self.obj = None

    def mock_loading_via_user_input_run(self, workspace, filename, run):
        self.model.load_workspace_from_filename = mock.Mock(
            return_value=(workspace, filename, run))
        self.view.set_run_edit_text("1234")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_model_and_view_initialized_to_contain_no_data(self):
        self.assertEqual(self.presenter.filenames, [])
        self.assertEqual(self.presenter.runs, [])
        self.assertEqual(self.presenter.workspaces, [])

        self.assertEqual(self.view.get_run_edit_text(), "")

    @run_test_with_and_without_threading
    def test_user_can_enter_a_run_and_load_it_in_single_file_mode(self):
        self.mock_loading_via_user_input_run([1, 2, 3], "EMU00001234.nxs", 1234)

        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["EMU00001234.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1, 2, 3]])

    @run_test_with_and_without_threading
    def test_warning_message_displayed_if_user_enters_multiple_files_in_single_file_mode(self):
        self.presenter.enable_multiple_files(False)
        self.view.warning_popup = mock.Mock()
        self.view.set_run_edit_text("1234,1235,1236")

        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
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
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonLoadData()
        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.presenter.enable_multiple_files(False)

        self.load_single_run()

    def tearDown(self):
        self.obj = None

    def load_single_run(self):
        self._loaded_run = 1234
        self._loaded_filename = "EMU00001234.nxs"
        self._loaded_workspace = [1, 2, 3]

        self.model.load_workspace_from_filename = mock.Mock(
            return_value=(self._loaded_workspace, self._loaded_filename, self._loaded_run))
        self.view.set_run_edit_text(str(self._loaded_run))
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

    def mock_model_to_load_single_run(self):
        self._loaded_run = 1234
        self._loaded_filename = "EMU00001234.nxs"
        self._loaded_workspace = [1, 2, 3]

        self.model.load_workspace_from_filename = mock.Mock(
            side_effect=zip((self._loaded_workspace, self._loaded_filename, self._loaded_run)))
        self.view.set_run_edit_text(str(self._loaded_run))

    def assert_model_has_not_changed(self):
        self.assertEqual(self.model.loaded_workspaces, [self._loaded_workspace])
        self.assertEqual(self.model.loaded_runs, [self._loaded_run])
        self.assertEqual(self.model.loaded_filenames, [self._loaded_filename])

    def assert_view_has_not_changed(self):
        self.assertEqual(self.view.get_run_edit_text(), str(self._loaded_run))

    def load_failure(self):
        raise ValueError("Error text")

    def mock_model_to_throw(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement buttons in single file mode (can only load one run at a time)
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_that_decrement_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001233.nxs"
        load_call_count = self.model.load_workspace_from_filename.call_count

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.model.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    @run_test_with_and_without_threading
    def test_that_increment_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001235.nxs"
        load_call_count = self.model.load_workspace_from_filename.call_count

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.model.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.model.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    @run_test_with_and_without_threading
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

    @run_test_with_and_without_threading
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

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        def load_failure():
            raise ValueError("Error text")

        self.model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assert_view_has_not_changed()

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assert_view_has_not_changed()

    def test_that_if_decrement_run_fails_warning_message_is_displayed(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_if_increment_run_fails_warning_message_is_displayed(self):
        for i in range(40):
            self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

            self.presenter.handle_increment_run()
            self.Runner(self.presenter._load_thread)

            if self.view.warning_popup.call_count != i + 1:
                print("failed on ", i)
            self.assertEqual(self.view.warning_popup.call_count, 1 + i)

    def test_that_if_decrement_run_fails_warning_message_is_displayed_with_no_threading(self):
        """These tests have to be constructed separately for no threading, as the thread_model forces the use
        of the message_box file; conversely the no threading code cannot do this as it would create Qt imports in the
        presenter. Instead it calls a warning pop-up in the view. This is how thread_model should operate as the view
        handles the parenting of the warning box correctly."""
        self.presenter._use_threading = False
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_if_increment_run_fails_warning_message_is_displayed_with_no_threading(self):
        self.presenter._use_threading = False
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
