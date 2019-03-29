# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from PyQt4 import QtGui

from mantid.py3compat import mock

from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.load_run_widget.load_run_model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_presenter import LoadRunWidgetPresenter
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.Common.muon_load_data import MuonLoadData


class LoadRunWidgetPresenterTest(unittest.TestCase):
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

        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext()
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)
        self.data_context.instrument = 'EMU'
        self.data = MuonLoadData()
        self.view = LoadRunWidgetView(parent=self.obj)
        self.model = LoadRunWidgetModel(self.data, self.context)
        self.presenter = LoadRunWidgetPresenter(self.view, self.model)
        self.presenter.set_current_instrument("EMU")

        patcher = mock.patch('Muon.GUI.Common.load_run_widget.load_run_model.load_utils')
        self.addCleanup(patcher.stop)
        self.load_utils_patcher = patcher.start()
        self.load_utils_patcher.exception_message_for_failed_files.return_value = ''

    def tearDown(self):
        self.obj = None

    def mock_loading_via_user_input_run(self, workspace, filename, run):
        self.load_utils_patcher.load_workspace_from_filename = mock.Mock(
            return_value=(workspace, run, filename))
        self.view.set_run_edit_text("1234")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    @run_test_with_and_without_threading
    def test_model_and_view_initialized_to_contain_no_data(self):
        self.assertEqual(self.presenter.filenames, [])
        self.assertEqual(self.presenter.runs, [])
        self.assertEqual(self.presenter.workspaces, [])

        self.assertEqual(self.view.get_run_edit_text(), "")

    @run_test_with_and_without_threading
    def test_user_can_enter_a_run_and_load_it_in_single_file_mode(self):
        self.mock_loading_via_user_input_run([1, 2, 3], "EMU00001234.nxs", 1234)

        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.filenames, ["EMU00001234.nxs"])
        self.assertEqual(self.presenter.runs, [[1234]])
        self.assertEqual(self.presenter.workspaces, [[1, 2, 3]])

    @run_test_with_and_without_threading
    def test_warning_message_displayed_if_user_enters_multiple_files_in_single_file_mode(self):
        self.view.warning_popup = mock.Mock()
        self.view.set_run_edit_text("1234,1235,1236")

        self.presenter.handle_run_changed_by_user()
        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.view.warning_popup.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
