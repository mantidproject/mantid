import sys
from Muon.GUI.Common.load_run_widget.model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData
import Muon.GUI.Common.muon_file_utils as fileUtils

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from PyQt4 import QtGui
from PyQt4.QtGui import QApplication

# global QApplication (get errors if > 1 instance in the code)
QT_APP = QApplication([])


class LoadRunWidgetLoadCurrentRunTest(unittest.TestCase):
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

    def load_failure(self):
        raise ValueError("Error text")

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonLoadData()
        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.data)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)

        self.model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3], "currentRun.nxs", 1234))
        self.view.warning_popup = mock.Mock()
        self.view.disable_load_buttons = mock.Mock()
        self.view.enable_load_buttons = mock.Mock()

        fileUtils.get_current_run_filename = mock.Mock(return_value="EMU0001234.nxs")

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_load_current_run_loads_run_into_model(self):
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["currentRun.nxs"])
        self.assertEqual(self.presenter.runs, [1234])
        self.assertEqual(self.presenter.workspaces, [[1, 2, 3]])

        self.assertEqual(self.model.current_run, 1234)

    @run_test_with_and_without_threading
    def test_load_current_run_correctly_displays_run_if_load_successful(self):
        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)
        self.assertEqual(self.view.get_run_edit_text(), "1234")

    def test_load_current_run_displays_error_message_if_fails_to_load(self):
        self.model.load_workspace_from_filename = mock.Mock(side_effect=self.load_failure)

        self.presenter.handle_load_current_run()
        self.Runner(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)

    @run_test_with_and_without_threading
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

    @run_test_with_and_without_threading
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

    @run_test_with_and_without_threading
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
