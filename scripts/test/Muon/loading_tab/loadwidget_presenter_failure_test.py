# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt4 import QtGui
import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common import mock_widget

from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.muon_data_context import MuonDataContext
import Muon.GUI.Common.utilities.muon_file_utils as file_utils

from Muon.GUI.MuonAnalysis.load_widget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.load_widget.load_widget_presenter import LoadWidgetPresenter


class LoadRunWidgetPresenterLoadFailTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            self._qapp.processEvents()

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = 'EMU'
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {'OutputWorkspace': [workspace_mock], 'MainFieldDirection': 'transverse'}

    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.popup_patcher = mock.patch('Muon.GUI.Common.thread_model.warning')
        self.addCleanup(self.popup_patcher.stop)
        self.popup_mock = self.popup_patcher.start()

        self.load_patcher = mock.patch('Muon.GUI.Common.load_file_widget.model.load_utils.load_workspace_from_filename')
        self.addCleanup(self.load_patcher.stop)
        self.load_mock = self.load_patcher.start()

        self.load_run_patcher = mock.patch(
            'Muon.GUI.Common.load_run_widget.load_run_model.load_utils.load_workspace_from_filename')
        self.addCleanup(self.load_run_patcher.stop)
        self.load_run_mock = self.load_run_patcher.start()

        self.data = MuonLoadData()
        self.context = MuonDataContext(self.data)
        self.context.instrument = 'EMU'
        self.load_file_view = BrowseFileWidgetView(self.obj)
        self.load_run_view = LoadRunWidgetView(self.obj)
        self.load_file_model = BrowseFileWidgetModel(self.data, self.context)
        self.load_run_model = LoadRunWidgetModel(self.data, self.context)

        self.model = LoadWidgetModel(self.data, self.context)
        self.view = LoadWidgetView(parent=self.obj, load_run_view=self.load_run_view,
                                   load_file_view=self.load_file_view)

        self.presenter = LoadWidgetPresenter(view=self.view, model=self.model)
        self.presenter.set_load_file_widget(BrowseFileWidgetPresenter(self.load_file_view, self.load_file_model))
        self.presenter.set_load_run_widget(LoadRunWidgetPresenter(self.load_run_view, self.load_run_model))
        self.presenter.load_run_widget.set_current_instrument('EMU')

        self.presenter.load_file_widget._view.warning_popup = mock.MagicMock()
        self.presenter.load_run_widget._view.warning_popup = mock.MagicMock()

        self.load_file_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:\\dir1\\EMU0001234.nxs"])
        self.workspace_mock = self.create_fake_workspace(1)
        self.load_mock.return_value = (self.workspace_mock, 1234, "C:\\dir1\\EMU0001234.nxs")
        self.load_run_mock.return_value = (self.workspace_mock, 1234, "C:\\dir1\\EMU0001234.nxs")

        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.mock_loading_to_throw()
        file_utils.get_current_run_filename = mock.Mock(return_value="EMU0001234.nxs")

    def tearDown(self):
        self.obj = None

    def load_failure(self):
        raise ValueError("Error text")

    def mock_loading_to_throw(self):
        self.load_mock.side_effect = self.load_failure
        self.load_run_mock.side_effect = self.load_failure

    def mock_disabling_buttons_in_run_and_file_widget(self):
        self.load_run_view.disable_load_buttons = mock.Mock()
        self.load_run_view.enable_load_buttons = mock.Mock()
        self.load_file_view.disable_load_buttons = mock.Mock()
        self.load_file_view.enable_load_buttons = mock.Mock()

    def assert_model_unchanged(self):
        self.assertEqual(self.presenter._model.filenames, ["C:\\dir1\\EMU0001234.nxs"])
        self.assertEqual(self.presenter._model.workspaces, [self.workspace_mock])
        self.assertEqual(self.presenter._model.runs, [[1234]])

    def assert_interface_unchanged(self):
        self.assertEqual(self.load_file_view.get_file_edit_text(), "C:\\dir1\\EMU0001234.nxs")
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1234")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : The interface should always revert to its previous state if a load fails from anywhere in the widget
    # and a warning should be shown to the user.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_if_load_fails_from_browse_that_model_and_interface_are_unchanged_from_previous_state(self):
        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assert_model_unchanged()
        self.assert_interface_unchanged()

    def test_that_if_load_fails_from_user_file_entry_that_model_and_interface_are_unchanged_from_previous_state(self):
        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assert_model_unchanged()
        self.assert_interface_unchanged()

    def test_that_if_load_fails_from_current_run_that_model_and_interface_are_unchanged_from_previous_state(self):
        self.presenter.load_run_widget.handle_load_current_run()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assert_model_unchanged()
        self.assert_interface_unchanged()

    def test_that_if_load_fails_from_user_run_entry_that_model_and_interface_are_unchanged_from_previous_state(self):
        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assert_model_unchanged()
        self.assert_interface_unchanged()

    def test_that_if_load_fails_from_browse_that_warning_is_displayed(self):
        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.presenter.load_file_widget._view.warning_popup.call_count, 1)

    def test_that_if_load_fails_from_user_file_entry_that_warning_is_displayed(self):
        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.presenter.load_file_widget._view.warning_popup.call_count, 1)

    def test_that_if_load_fails_from_current_run_that_warning_is_displayed(self):
        self.presenter.load_run_widget.handle_load_current_run()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.presenter.load_run_widget._view.warning_popup.call_count, 1)

    def test_that_if_load_fails_from_user_run_entry_that_warning_is_displayed(self):
        self.presenter.load_run_widget._view.run_edit.setText('1239')
        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.presenter.load_run_widget._view.warning_popup.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
