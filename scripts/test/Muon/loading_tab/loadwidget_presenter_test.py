# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt4 import QtGui
import unittest

from mantid.api import FileFinder
from mantid.py3compat import mock
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from Muon.GUI.Common.load_file_widget.presenter import BrowseFileWidgetPresenter
from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common import mock_widget

from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.muon_context import MuonContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_group_pair_context import MuonGroupPairContext
import Muon.GUI.Common.utilities.muon_file_utils as file_utils

from Muon.GUI.MuonAnalysis.load_widget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.load_widget.load_widget_presenter import LoadWidgetPresenter


class LoadRunWidgetPresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            self._qapp.processEvents()

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonLoadData()
        self.data_context = MuonDataContext(self.data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext()
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)
        self.context.instrument = 'EMU'
        self.load_file_view = BrowseFileWidgetView(self.obj)
        self.load_run_view = LoadRunWidgetView(self.obj)
        self.load_file_model = BrowseFileWidgetModel(self.data, self.context)
        self.load_run_model = LoadRunWidgetModel(self.data, self.context)

        self.presenter = LoadWidgetPresenter(
            LoadWidgetView(parent=self.obj, load_file_view=self.load_file_view, load_run_view=self.load_run_view),
            LoadWidgetModel(self.data, self.context))
        self.presenter.set_load_file_widget(BrowseFileWidgetPresenter(self.load_file_view, self.load_file_model))
        self.presenter.set_load_run_widget(LoadRunWidgetPresenter(self.load_run_view, self.load_run_model))

        self.filepath = FileFinder.findRuns('MUSR00022725.nxs')[0]

        self.load_patcher = mock.patch('Muon.GUI.Common.load_file_widget.model.load_utils.load_workspace_from_filename')
        self.addCleanup(self.load_patcher.stop)
        self.load_mock = self.load_patcher.start()

        self.load_run_patcher = mock.patch(
            'Muon.GUI.Common.load_run_widget.load_run_model.load_utils.load_workspace_from_filename')
        self.addCleanup(self.load_run_patcher.stop)
        self.load_run_mock = self.load_run_patcher.start()

        self.mock_workspace = self.create_fake_workspace(1)
        self.mock_loading_from_browse(self.mock_workspace, "C:\dir1\dir2\dir3\EMU0001234.nxs", 1234)
        file_utils.get_current_run_filename = mock.Mock(return_value="C:\dir1\dir2\dir3\EMU0001234.nxs")

        self.presenter.load_file_widget._view.warning_popup = mock.MagicMock()
        self.presenter.load_run_widget._view.warning_popup = mock.MagicMock()

        self.popup_patcher = mock.patch('Muon.GUI.Common.thread_model.warning')
        self.addCleanup(self.popup_patcher.stop)
        self.popup_mock = self.popup_patcher.start()

    def tearDown(self):
        self.obj = None

    def create_fake_workspace(self, name):
        workspace_mock = mock.MagicMock()
        instrument_mock = mock.MagicMock()
        instrument_mock.getName.return_value = 'EMU'
        workspace_mock.workspace.getInstrument.return_value = instrument_mock

        return {'OutputWorkspace': [workspace_mock], 'MainFieldDirection': 'transverse'}

    def mock_loading_from_browse(self, workspace, filename, run):
        self.load_file_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=[filename])
        self.load_mock.return_value = (workspace, run, filename)
        self.load_run_mock.return_value = (workspace, run, filename)

    def mock_loading_from_current_run(self, workspace, filename, run):
        file_utils.get_current_run_filename = mock.Mock(return_value=filename)
        self.load_run_mock.return_value = (workspace, run, filename)
        self.load_mock.return_value = (workspace, run, filename)

    def mock_user_input_single_run(self, workspace, filename, run):
        self.load_run_view.get_run_edit_text = mock.Mock(return_value=str(run))
        self.load_run_mock.return_value = (workspace, run, filename)
        self.load_mock.return_value = (workspace, run, filename)

    def mock_user_input_single_file(self, workspace, filename, run):
        self.load_run_mock.return_value = (workspace, run, filename)
        self.load_mock.return_value = (workspace, run, filename)

        self.load_file_view.get_file_edit_text = mock.Mock(
            return_value=filename)

    def mock_disabling_buttons_in_run_and_file_widget(self):
        self.load_run_view.disable_load_buttons = mock.Mock()
        self.load_run_view.enable_load_buttons = mock.Mock()
        self.load_file_view.disable_load_buttons = mock.Mock()
        self.load_file_view.enable_load_buttons = mock.Mock()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : The interface should correctly load/display data via the run widget (load current run and user input run)
    # and via the file widget (browse button or user enters file)
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_loading_single_file_via_browse_sets_model_data(self):
        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.presenter._model.filenames, ["C:\dir1\dir2\dir3\EMU0001234.nxs"])
        self.assertEqual(self.presenter._model.workspaces, [self.mock_workspace])
        self.assertEqual(self.presenter._model.runs, [[1234]])

    def test_that_loading_single_file_via_browse_sets_run_and_file_view(self):
        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.load_file_view.get_file_edit_text(), "C:\dir1\dir2\dir3\EMU0001234.nxs")
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1234")

    def test_that_loading_via_browse_disables_all_load_buttons(self):
        self.mock_disabling_buttons_in_run_and_file_widget()

        self.presenter.load_file_widget.on_browse_button_clicked()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.load_file_view.disable_load_buttons.call_count, 2)
        self.assertEqual(self.load_file_view.enable_load_buttons.call_count, 2)
        self.assertEqual(self.load_run_view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.load_run_view.enable_load_buttons.call_count, 1)

    def test_that_loading_single_file_via_user_input_file_is_loaded_into_model_correctly(self):
        self.mock_user_input_single_file(self.mock_workspace, "C:\dir1\dir2\dir3\EMU0001111.nxs", 1111)

        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.presenter._model.filenames, ["C:\dir1\dir2\dir3\EMU0001111.nxs"])
        self.assertEqual(self.presenter._model.workspaces, [self.mock_workspace])
        self.assertEqual(self.presenter._model.runs, [[1111]])

    def test_that_loading_single_file_via_user_input_file_is_displayed_correctly_in_the_interface(self):
        self.mock_user_input_single_file(self.mock_workspace, "C:\dir1\dir2\dir3\EMU0001111.nxs", 1111)

        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.load_file_view.get_file_edit_text(), "C:\dir1\dir2\dir3\EMU0001111.nxs")
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1111")

    def test_that_loading_via_user_input_file_disables_all_load_buttons(self):
        self.mock_user_input_single_file([1], "C:\dir1\dir2\dir3\EMU0001111.nxs", 1111)

        self.mock_disabling_buttons_in_run_and_file_widget()

        self.presenter.load_file_widget.handle_file_changed_by_user()
        self.wait_for_thread(self.presenter.load_file_widget._load_thread)

        self.assertEqual(self.load_file_view.disable_load_buttons.call_count, 2)
        self.assertEqual(self.load_file_view.enable_load_buttons.call_count, 2)
        self.assertEqual(self.load_run_view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.load_run_view.enable_load_buttons.call_count, 1)

    def test_that_loading_via_load_current_run_is_loaded_into_model_correctly(self):
        self.mock_loading_from_current_run(self.mock_workspace, "\\\\EMU\\data\\EMU0083420.nxs", 83420)

        self.presenter.load_run_widget.handle_load_current_run()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.presenter._model.filenames, ["\\\\EMU\\data\\EMU0083420.nxs"])
        self.assertEqual(self.presenter._model.workspaces, [self.mock_workspace])
        self.assertEqual(self.presenter._model.runs, [[83420]])

    def test_that_loading_via_load_current_run_is_displayed_correctly_in_the_interface(self):
        self.mock_loading_from_current_run(self.mock_workspace, "\\\\EMU\\data\\EMU0083420.nxs", 83420)

        self.presenter.load_run_widget.handle_load_current_run()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.load_file_view.get_file_edit_text(), "\\\\EMU\\data\\EMU0083420.nxs")
        self.assertEqual(self.load_run_view.get_run_edit_text(), "83420")

    def test_that_loading_via_load_current_run_disables_all_load_buttons(self):
        self.mock_loading_from_current_run(self.mock_workspace, "\\\\EMU\\data\\EMU0083420.nxs", 83420)
        self.mock_disabling_buttons_in_run_and_file_widget()

        self.presenter.load_run_widget.handle_load_current_run()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.load_file_view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.load_file_view.enable_load_buttons.call_count, 1)
        self.assertEqual(self.load_run_view.disable_load_buttons.call_count, 3)
        self.assertEqual(self.load_run_view.enable_load_buttons.call_count, 2)

    def test_that_user_input_run_is_loaded_into_model_correctly(self):
        self.mock_user_input_single_run(self.mock_workspace, "EMU00012345.nxs", 12345)
        self.presenter.load_run_widget.set_current_instrument('EMU')

        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.presenter._model.filenames, ["EMU00012345.nxs"])
        self.assertEqual(self.presenter._model.workspaces, [self.mock_workspace])
        self.assertEqual(self.presenter._model.runs, [[12345]])

    def test_that_user_input_run_is_displayed_correctly_in_the_interface(self):
        self.mock_user_input_single_run(self.mock_workspace, "EMU00012345.nxs", 12345)
        self.presenter.load_run_widget.set_current_instrument('EMU')

        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.load_file_view.get_file_edit_text(), "EMU00012345.nxs")
        self.assertEqual(self.load_run_view.get_run_edit_text(), "12345")

    def test_that_loading_via_user_input_run_disables_all_load_buttons(self):
        self.mock_user_input_single_run(self.mock_workspace, "EMU00012345.nxs", 12345)
        self.presenter.load_run_widget.set_current_instrument('EMU')

        self.mock_disabling_buttons_in_run_and_file_widget()

        self.presenter.load_run_widget.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter.load_run_widget._load_thread)

        self.assertEqual(self.load_file_view.disable_load_buttons.call_count, 1)
        self.assertEqual(self.load_file_view.enable_load_buttons.call_count, 1)
        self.assertEqual(self.load_run_view.disable_load_buttons.call_count, 3)
        self.assertEqual(self.load_run_view.enable_load_buttons.call_count, 2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
