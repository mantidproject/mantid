import sys
import os
import time

from Muon.GUI.MuonAnalysis.loadwidget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_presenter import LoadWidgetPresenter

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView

from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView

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


class LoadRunWidgetPresenterMultipleFileTest(unittest.TestCase):
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
        self.obj = QtGui.QWidget()
        self.data = MuonLoadData()
        self.load_file_view = BrowseFileWidgetView(self.obj)
        self.load_run_view = LoadRunWidgetView(self.obj)
        self.load_file_model = BrowseFileWidgetModel(self.data)
        self.load_run_model = LoadRunWidgetModel(self.data)

        self.view = LoadWidgetView(parent=self.obj, load_file_view=self.load_file_view, load_run_view=self.load_run_view)
        self.presenter = LoadWidgetPresenter(self.view, LoadWidgetModel(self.data))
        self.presenter.set_load_file_widget(BrowseFileWidgetPresenter(self.load_file_view, self.load_file_model))
        self.presenter.set_load_run_widget(LoadRunWidgetPresenter(self.load_run_view, self.load_run_model))

        self.view.multiple_loading_check.setCheckState(1)
        self.presenter.handle_multiple_files_option_changed()

        self.runs = [1234 + i for i in range(10)]
        self.workspaces = [[run] for run in self.runs]
        self.filenames = ["C:\dir1\dir2\EMU000" + str(run) + ".nxs" for run in self.runs]

    def tearDown(self):
        self.obj = None

    def mock_loading_from_browse(self, workspaces, filenames, runs):
        self.load_file_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=filenames)
        self.load_file_model.load_workspace_from_filename = mock.Mock(
            side_effect=zip(workspaces, runs))

    def mock_loading_from_current_run(self, workspace, filename, run):
        self.load_run_model.load_workspace_from_filename = mock.Mock(
            return_value=(workspace, filename, run))

    def mock_user_input_multiple_runs(self, user_input, workspaces, filenames, runs):
        self.load_run_view.get_run_edit_text = mock.Mock(return_value=user_input)
        self.load_run_model.load_workspace_from_filename = mock.Mock(
            side_effect=zip(workspaces, filenames, runs))

    def mock_user_input_multiple_files(self, workspaces, filenames, runs):
        self.load_file_view.get_file_edit_text = mock.Mock(
            return_value=";".join(filenames))
        self.load_file_model.load_workspace_from_filename = mock.Mock(
            side_effect=zip(workspaces, runs))

    def assert_model_contains_correct_loaded_data(self):
        # use sorted due to threads finishing at different times
        self.assertEqual(sorted(self.presenter._model.filenames), sorted(self.filenames))
        self.assertEqual(sorted(self.presenter._model.workspaces), sorted(self.workspaces))
        self.assertEqual(sorted(self.presenter._model.runs), sorted(self.runs))

    def assert_interface_contains_correct_runs_and_files(self):
        self.assertEqual(self.load_file_view.get_file_edit_text(), ";".join(self.filenames))
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1234-1243")

    def mock_disabling_buttons_in_run_and_file_widget(self):
        self.load_run_view.disable_load_buttons = mock.Mock()
        self.load_run_view.enable_load_buttons = mock.Mock()
        self.load_file_view.disable_load_buttons = mock.Mock()
        self.load_file_view.enable_load_buttons = mock.Mock()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : The loading of multiple files is supported by the widget
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_loading_multiple_files_via_browse_sets_model_and_interface_correctly(self):
        self.mock_loading_from_browse(self.workspaces, self.filenames, self.runs)

        self.presenter.load_file_widget.on_browse_button_clicked()
        self.Runner(self.presenter.load_file_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()

    def test_that_loading_multiple_files_via_user_input_files_sets_model_and_interface_correctly(self):
        self.mock_user_input_multiple_files(self.workspaces, self.filenames, self.runs)

        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.Runner(self.presenter.load_file_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()

    def test_that_loading_multiple_files_via_user_input_runs_sets_model_and_interface_correctly(self):
        self.mock_user_input_multiple_runs("1234-1243", self.workspaces, self.filenames, self.runs)

        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.Runner(self.presenter.load_run_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()

    def test_that_loading_multiple_files_then_allows_loading_via_increment_and_decrement(self):
        pass


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
