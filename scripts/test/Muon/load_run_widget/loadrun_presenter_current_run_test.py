# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import patch

import Muon.GUI.Common.utilities.muon_file_utils as fileUtils
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests
from Muon.GUI.Common.test_helpers.general_test_helpers import EMPTY_PERIOD_INFO_LIST
from Muon.GUI.Common.thread_model import ThreadModel, ThreadModelWorker
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication, QWidget


# this class is required to keep track of error signal emissions since the output is garbage collected by the time
# we reach the equal assertion


class MockSignalHandler(object):
    def __init__(self, parent=None):
        self.call_count = 0

    def signalReceived(self):
        self.call_count+=1


@start_qapplication
class LoadRunWidgetLoadCurrentRunTest(unittest.TestCase):
    def run_test_with_and_without_threading(test_function):
        def run_twice(self):
            test_function(self)
            self.setUp()
            self.presenter._use_threading = False
            test_function(self)

        return run_twice

    @staticmethod
    def load_failure(self):
        raise ValueError("Error text")

    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            QApplication.sendPostedEvents()

    def create_fake_workspace(self):
        return {'MainFieldDirection': 'transverse'}

    def create_mock_signal_handler(self):
        self.thread_model = ThreadModel(self.model)
        self.thread_model_worker = ThreadModelWorker(self.thread_model)
        self.thread_model_worker.signals.error = mock.Mock()
        self.signal_handler = MockSignalHandler()
        self.thread_model_worker.signals.error.connect(self.signal_handler.signalReceived())

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.data_context.instrument = 'EMU'
        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3], "currentRun.nxs", 1234, False,
                                                                          EMPTY_PERIOD_INFO_LIST))
        self.view.warning_popup = mock.Mock()
        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()

        self.presenter.set_current_instrument("EMU")

        fileUtils.get_current_run_filename = mock.Mock(return_value="EMU0001234.nxs")

        patcher = mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.exception_message_for_failed_files.return_value = ''

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_load_current_run_loads_run_into_model(self):
        workspace = self.create_fake_workspace()
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=(workspace, 1234, "currentRun.nxs"
                                                                                       , False, EMPTY_PERIOD_INFO_LIST))
        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["currentRun.nxs"])
        self.assertEqual(self.presenter.runs, [[1234]])
        self.assertEqual(self.presenter.workspaces, [workspace])

        self.assertEqual(self.model._data_context.current_runs, [[1234]])

    @run_test_with_and_without_threading
    def test_load_current_run_correctly_displays_run_if_load_successful(self):
        workspace = self.create_fake_workspace()
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=(workspace, 1234, "1234.nxs",
                                                                                       False, EMPTY_PERIOD_INFO_LIST))
        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_run_edit_text(), '1234')

    def test_load_current_run_emits_error_signal_if_fails_to_load(self):
        self.create_mock_signal_handler()
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.signal_handler.call_count, 1)

    @run_test_with_and_without_threading
    # the following patch is required because the warning popup originates from thread_model in this case
    @patch("Muon.GUI.Common.load_run_widget.load_run_presenter.thread_model.warning")
    def test_load_current_run_reverts_to_previous_data_if_fails_to_load(self, warning_mock):
        # set up previous data
        workspace = self.create_fake_workspace()
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=(workspace, 1234, "1234.nxs",
                                                                                       False, EMPTY_PERIOD_INFO_LIST))
        self.view.set_run_edit_text("1234")
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)
        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["1234.nxs"])
        self.assertEqual(self.presenter.runs, [[1234]])
        self.assertEqual(self.presenter.workspaces, [workspace])

    @run_test_with_and_without_threading
    def test_load_current_run_clears_previous_data_if_load_succeeds(self):
        # set up previous data
        workspace = self.create_fake_workspace()
        self.load_utils_patcher.return_value = (workspace, "1234.nxs", 1234)
        self.view.set_run_edit_text("1234")
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=(workspace, 9999, "9999.nxs",
                                                                                       False, EMPTY_PERIOD_INFO_LIST))
        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.get_run_edit_text(), "9999")
        self.assertEqual(self.presenter.filenames, ["9999.nxs"])
        self.assertEqual(self.presenter.runs, [[9999]])
        self.assertEqual(self.presenter.workspaces, [workspace])

    @run_test_with_and_without_threading
    # the following patch is required because the warning popup also originates from thread_model in this case
    @patch("Muon.GUI.Common.load_run_widget.load_run_presenter.thread_model.warning")
    def test_load_current_run_emits_error_signal_if_incrementing_past_current_run(self, warning_mock):
        self.create_mock_signal_handler()

        # set up current run
        workspace = self.create_fake_workspace()
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=(workspace, 1234, "1234.nxs",
                                                                                       False, EMPTY_PERIOD_INFO_LIST))
        self.view.set_run_edit_text("1234")
        self.presenter.handle_load_current_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.signal_handler.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
