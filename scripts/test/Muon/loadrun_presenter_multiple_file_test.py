import sys
import six

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

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


class LoadRunWidgetIncrementDecrementMultipleFileModeTest(unittest.TestCase):
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
        self.view = LoadRunWidgetView(self.obj)
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

        self.presenter.enable_multiple_files(True)

    def tearDown(self):
        self.obj = None

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

    @run_test_with_and_without_threading
    def test_that_providing_no_runs_leaves_model_and_view_empty(self):
        self.view.set_run_edit_text("")
        self.presenter.handle_run_changed_by_user()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_increment_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_decrement_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_decrement_run_decrements_the_upper_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(return_value=([1], "file1.nxs", 1))

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file1.nxs", "file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[1], [2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [1, 2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "1-4")

    @run_test_with_and_without_threading
    def test_that_increment_run_increments_the_lower_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(return_value=([5], "file5.nxs", 5))

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs", "file5.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4], [5]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4, 5])

        self.assertEqual(self.view.get_run_edit_text(), "2-5")

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        def load_failure():
            raise ValueError("Error text")

        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        def load_failure():
            raise ValueError("Error text")

        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        six.assertCountEqual(self, self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        six.assertCountEqual(self, self.model.loaded_workspaces, [[2], [3], [4]])
        six.assertCountEqual(self, self.model.loaded_runs, [2, 3, 4])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    # These tests have to be constructed separately for no threading, as the thread_model forces the use
    # of the message_box file; conversely the no threading code cannot do this as it would create Qt imports in the
    # presenter. Instead it calls a warning pop-up in the view. This is how thread_model should operate as the view
    # handles the parenting of the warning box correctly.

    def test_that_if_increment_run_fails_warning_message_is_displayed(self):
        with mock.patch("Muon.GUI.Common.message_box.warning") as mock_warning:
            self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
            self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

            self.presenter.handle_increment_run()
            self.Runner(self.presenter._load_thread)

            self.assertEqual(mock_warning.call_count, 1)

    def test_that_if_decrement_run_fails_warning_message_is_displayed(self):
        with mock.patch("Muon.GUI.Common.message_box.warning") as mock_warning:

            self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
            self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

            self.presenter.handle_decrement_run()
            self.Runner(self.presenter._load_thread)

            self.assertEqual(mock_warning.call_count, 1)

    def test_that_if_increment_run_fails_with_no_threading_a_warning_message_is_displayed(self):
        self.presenter._use_threading = False
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_if_decrement_run_fails_with_no_threading_a_warning_message_is_displayed(self):
        self.presenter._use_threading = False
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
