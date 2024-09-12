# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import time

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication, QWidget
from mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


@start_qapplication
class LoadRunWidgetIncrementDecrementMultipleFileModeTest(unittest.TestCase):
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
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.data_context.instrument = "EMU"

        self.view = LoadRunWidgetView(self.obj)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

        self.presenter.set_current_instrument("EMU")

        patcher = mock.patch("mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_model.load_utils")
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.exception_message_for_failed_files.return_value = ""

    def tearDown(self):
        self.obj = None

    def load_runs(self, runs, filenames, workspaces):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            side_effect=iter(zip(workspaces, runs, filenames, [False] * len(filenames)))
        )
        run_string = ",".join([str(run) for run in runs])
        self.view.set_run_edit_text(run_string)
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

    def assert_model_empty(self):
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_runs, [])

    def assert_view_empty(self):
        self.assertEqual(self.view.get_run_edit_text(), "")

    @staticmethod
    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement run buttons in "multiple file" mode
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_that_providing_no_runs_leaves_model_and_view_empty(self):
        self.view.set_run_edit_text("")
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_increment_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_decrement_run_does_nothing_if_no_runs_loaded(self):
        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_view_empty()
        self.assert_model_empty()

    @run_test_with_and_without_threading
    def test_that_decrement_run_decrements_the_upper_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=([1], 1, "file1.nxs", False))

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["file1.nxs", "file2.nxs", "file3.nxs", "file4.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [[1], [2], [3], [4]])
        self.assertCountEqual(self.model.loaded_runs, [[1], [2], [3], [4]])

        self.assertEqual(self.view.get_run_edit_text(), "1")

    @run_test_with_and_without_threading
    def test_that_increment_run_increments_the_lower_end_of_the_range_of_loaded_runs(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=([5], 5, "file5.nxs", False))

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs", "file5.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [[2], [3], [4], [5]])
        self.assertCountEqual(self.model.loaded_runs, [[2], [3], [4], [5]])

        self.assertEqual(self.view.get_run_edit_text(), "5")

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [[2], [3], [4]])
        self.assertCountEqual(self.model.loaded_runs, [[2], [3], [4]])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertCountEqual(self.model.loaded_filenames, ["file2.nxs", "file3.nxs", "file4.nxs"])
        self.assertCountEqual(self.model.loaded_workspaces, [[2], [3], [4]])
        self.assertCountEqual(self.model.loaded_runs, [[2], [3], [4]])

        self.assertEqual(self.view.get_run_edit_text(), "2-4")

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_warning_message_is_displayed(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_warning_message_is_displayed(self):
        self.load_runs([2, 3, 4], ["file2.nxs", "file3.nxs", "file4.nxs"], [[2], [3], [4]])
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
