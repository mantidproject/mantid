# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest
from PyQt4 import QtGui

from mantid.py3compat import mock

from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from test.Muon.context_setup import setup_context_for_tests


class LoadRunWidgetIncrementDecrementSingleFileModeTest(unittest.TestCase):
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
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        setup_context_for_tests(self)

        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.presenter.set_current_instrument("EMU")

        patcher = mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.exception_message_for_failed_files.return_value = ''

        self.load_single_run()

    def tearDown(self):
        self.obj = None

    def load_single_run(self):
        self._loaded_run = 1234
        self._loaded_filename = "EMU00001234.nxs"
        self._loaded_workspace = {'MainFieldDirection': 'transverse'}

        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            return_value=(self._loaded_workspace, self._loaded_run, self._loaded_filename))
        self.view.set_run_edit_text(str(self._loaded_run))
        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

    def assert_model_has_not_changed(self):
        self.assertEqual(self.model.loaded_workspaces, [self._loaded_workspace])
        self.assertEqual(self.model.loaded_runs, [[self._loaded_run]])
        self.assertEqual(self.model.loaded_filenames, [self._loaded_filename])

    def assert_view_has_not_changed(self):
        self.assertEqual(self.view.get_run_edit_text(), str(self._loaded_run))

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
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=({'MainFieldDirection': 'transverse'}, new_run, new_filename))

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [self._loaded_filename, new_filename])
        self.assertEqual(self.presenter.runs, [[self._loaded_run], [new_run]])
        self.assertEqual(self.presenter.workspaces, [self._loaded_workspace, {'MainFieldDirection': 'transverse'}])

        self.assertEqual(self.view.get_run_edit_text(), '1233')

    @run_test_with_and_without_threading
    def test_that_increment_run_loads_the_data_correctly(self):
        new_run = self._loaded_run + 1
        new_filename = "EMU00001235.nxs"
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(return_value=({'MainFieldDirection': 'transverse'}, new_run, new_filename))

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, [self._loaded_filename, new_filename])
        self.assertEqual(self.presenter.runs, [[self._loaded_run], [new_run]])
        self.assertEqual(self.presenter.workspaces, [self._loaded_workspace, {'MainFieldDirection': 'transverse'}])

        self.assertEqual(self.view.get_run_edit_text(), '1235')

    @run_test_with_and_without_threading
    def test_that_if_decrement_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_decrement_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assertEqual(self.view.get_run_edit_text(), '')

    @run_test_with_and_without_threading
    def test_that_if_increment_run_fails_the_data_are_returned_to_previous_state(self):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_increment_run()
        self.wait_for_thread(self.presenter._load_thread)

        self.assert_model_has_not_changed()
        self.assertEqual(self.view.get_run_edit_text(), '')

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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)