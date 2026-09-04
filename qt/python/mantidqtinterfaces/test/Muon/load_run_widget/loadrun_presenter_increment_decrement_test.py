# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
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
class LoadRunWidgetIncrementDecrementSingleFileModeTest(unittest.TestCase):
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

        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.presenter.set_current_instrument("EMU")

        patcher = mock.patch("mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_model.load_utils")
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.exception_message_for_failed_files.return_value = ""

        self.load_single_run()

    def tearDown(self):
        self.obj = None

    def load_single_run(self):
        self._loaded_run = 1234
        self._loaded_filename = "EMU00001234.nxs"
        self._loaded_workspace = {"MainFieldDirection": "transverse"}

        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            return_value=(self._loaded_workspace, self._loaded_run, self._loaded_filename, False)
        )
        self.view.set_run_edit_text(str(self._loaded_run))
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

    def assert_model_has_not_changed(self):
        self.assertEqual(self.model.loaded_workspaces, [self._loaded_workspace])
        self.assertEqual(self.model.loaded_runs, [[self._loaded_run]])
        self.assertEqual(self.model.loaded_filenames, [self._loaded_filename])

    def assert_view_has_not_changed(self):
        self.assertEqual(self.view.get_run_edit_text(), str(self._loaded_run))

    @staticmethod
    def load_failure(self):
        raise ValueError("Error text")

    def mock_model_to_throw(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement buttons in single file mode (can only load one run at a time)
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_that_decrement_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001233.nxs"
        load_call_count = self.load_utils_patcher.load_workspace_from_filename.call_count

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.load_utils_patcher.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    @run_test_with_and_without_threading
    def test_that_increment_run_attempts_to_load_the_correct_run(self):
        new_filename = "EMU00001235.nxs"
        load_call_count = self.load_utils_patcher.load_workspace_from_filename.call_count

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.load_utils_patcher.load_workspace_from_filename.call_count, load_call_count + 1)
        filename = self.load_utils_patcher.load_workspace_from_filename.call_args[0][0]
        self.assertEqual(os.path.basename(filename), new_filename)

    @run_test_with_and_without_threading
    def test_that_decrement_run_loads_the_data_correctly(self):
        new_run = self._loaded_run - 1
        new_filename = "EMU00001233.nxs"
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            return_value=({"MainFieldDirection": "transverse"}, new_run, new_filename, False)
        )

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [self._loaded_filename, new_filename])
        self.assertEqual(self.presenter.runs, [[self._loaded_run], [new_run]])
        self.assertEqual(self.presenter.workspaces, [self._loaded_workspace, {"MainFieldDirection": "transverse"}])

        self.assertEqual(self.view.get_run_edit_text(), "1233")

    @run_test_with_and_without_threading
    def test_that_increment_run_loads_the_data_correctly(self):
        new_run = self._loaded_run + 1
        new_filename = "EMU00001235.nxs"
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            return_value=({"MainFieldDirection": "transverse"}, new_run, new_filename, False)
        )

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [self._loaded_filename, new_filename])
        self.assertEqual(self.presenter.runs, [[self._loaded_run], [new_run]])
        self.assertEqual(self.presenter.workspaces, [self._loaded_workspace, {"MainFieldDirection": "transverse"}])

        self.assertEqual(self.view.get_run_edit_text(), "1235")

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assertEqual(self.view.get_run_edit_text(), "1234")

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assertEqual(self.view.get_run_edit_text(), "1234")

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_warning_message_is_displayed(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_warning_message_is_displayed(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Re-entrancy guards. Two loads of the same run running concurrently collide on the ADS names
    # minted by load_utils.create_load_algorithm, which can deadlock the GUI thread against the loader.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_a_second_load_request_is_ignored_while_a_load_thread_is_in_progress(self):
        self.presenter.create_load_thread = mock.Mock(return_value=mock.Mock())

        self.presenter.handle_increment_run()
        self.presenter.handle_increment_run()

        self.assertEqual(self.presenter.create_load_thread.call_count, 1)

    def test_that_a_load_request_is_allowed_again_once_the_load_thread_has_finished(self):
        self.presenter.create_load_thread = mock.Mock(return_value=mock.Mock())

        self.presenter.load_runs(["EMU00001235.nxs"])
        self.assertEqual(self.presenter.create_load_thread.call_count, 1)

        self.presenter._load_thread = None
        self.presenter.load_runs(["EMU00001236.nxs"])

        self.assertEqual(self.presenter.create_load_thread.call_count, 2)

    def test_that_enable_loading_does_not_enable_the_buttons_while_a_load_thread_is_in_progress(self):
        self.view.enable_load_buttons = mock.Mock()
        self.presenter._load_thread = mock.Mock()

        self.presenter.enable_loading()

        self.assertEqual(self.view.enable_load_buttons.call_count, 0)

    def test_that_enable_loading_enables_the_buttons_once_the_load_thread_has_finished(self):
        self.view.enable_load_buttons = mock.Mock()
        self.presenter._load_thread = None

        self.presenter.enable_loading()

        self.assertEqual(self.view.enable_load_buttons.call_count, 1)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
