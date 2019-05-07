# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from PyQt4 import QtGui

from mantid import ConfigService
from mantid.api import FileFinder
from mantid.py3compat import mock

from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests
from Muon.GUI.MuonAnalysis.load_widget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.load_widget.load_widget_presenter import LoadWidgetPresenter
from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView


class LoadRunWidgetPresenterMultipleFileTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            self._qapp.processEvents()

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()
        ConfigService['default.instrument'] = 'MUSR'

        setup_context_for_tests(self)
        self.context.instrument = 'MUSR'
        self.load_file_view = BrowseFileWidgetView(self.obj)
        self.load_run_view = LoadRunWidgetView(self.obj)
        self.load_file_model = BrowseFileWidgetModel(self.loaded_data, self.context)
        self.load_run_model = LoadRunWidgetModel(self.loaded_data, self.context)

        self.view = LoadWidgetView(parent=self.obj, load_file_view=self.load_file_view,
                                   load_run_view=self.load_run_view)
        self.presenter = LoadWidgetPresenter(self.view, LoadWidgetModel(self.loaded_data, self.context))
        self.presenter.set_load_file_widget(BrowseFileWidgetPresenter(self.load_file_view, self.load_file_model))
        self.presenter.set_load_run_widget(LoadRunWidgetPresenter(self.load_run_view, self.load_run_model))

        self.presenter.load_file_widget._view.warning_popup = mock.MagicMock()
        self.presenter.load_run_widget._view.warning_popup = mock.MagicMock()

        self.view.multiple_loading_check.setCheckState(1)
        self.presenter.handle_multiple_files_option_changed()

        self.runs = [15196, 15197]
        self.workspaces = [self.create_fake_workspace(1) for _ in self.runs]
        self.filenames = FileFinder.findRuns('MUSR00015196.nxs, MUSR00015197.nxs')

    def tearDown(self):
        self.obj = None

    def assert_model_contains_correct_loaded_data(self):
        # use sorted due to threads finishing at different times
        self.assertEqual(sorted(self.presenter._model.filenames), ['Co-added'])
        self.assertEqual(sorted(self.presenter._model.runs), [sorted(self.runs)])

    def assert_interface_contains_correct_runs_and_files(self):
        self.assertEqual(self.load_file_view.get_file_edit_text(), ";".join(['Co-added']))
        self.assertEqual(self.load_run_view.get_run_edit_text(), "15196-15197")

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = 'MUSR'
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {'OutputWorkspace': [workspace_mock]}

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : The loading of multiple files is supported by the widget
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_loading_multiple_files_via_browse_sets_model_and_interface_correctly(self):
        self.load_file_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=self.filenames)

        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()

    def test_that_loading_multiple_files_via_user_input_files_sets_model_and_interface_correctly(self):
        self.load_file_view.file_path_edit.setText(';'.join(self.filenames))

        self.load_file_view.file_path_edit.returnPressed.emit()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()

    def test_that_loading_multiple_files_via_user_input_runs_sets_model_and_interface_correctly(self):
        self.load_run_view.run_edit.setText("15196-97")

        self.load_run_view.run_edit.returnPressed.emit()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assert_model_contains_correct_loaded_data()
        self.assert_interface_contains_correct_runs_and_files()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
