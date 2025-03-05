# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import time

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication, QWidget

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadRunWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class LoadRunWidgetPresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model and thread_model.worker:
            while thread_model.worker.is_alive():
                time.sleep(0.1)
            QApplication.sendPostedEvents()

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_ea_tests(self)

        self.view = mock.create_autospec(LoadRunWidgetView, instance=True)
        self.model = mock.Mock(LoadRunWidgetModel, autospec=True)
        self.presenter = LoadRunWidgetPresenterEA(self.view, self.model)

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_initialized_to_contain_no_data(self):
        self.assertEqual(self.presenter.run_list, [])

    def test_user_can_enter_a_run_and_load_it_in_single_run_mode(self):
        """
        Tests a user inputting a single run
        """
        self.view.get_run_edit_text.return_value = "5555"
        self.model.execute = mock.Mock()

        self.model._data_context = mock.Mock(current_runs=[5555])
        self.presenter.handle_run_changed_by_user()

        self.model.get_latest_loaded_run.return_value = 5555
        self.model._loaded_data_store = mock.Mock(run=5555)
        self.model._directory = mock.Mock()
        self.model.loaded_runs = [[5555]]

        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.runs, [[5555]])

    def test_clear_loaded_data(self):
        """
        Tests that clearing loaded data sends calls to clear both
        the loaded_data in the model and the view
        """
        self.presenter.clear_loaded_data()

        self.assertEqual(self.view.clear.call_count, 1)
        self.assertEqual(self.model.clear_loaded_data.call_count, 1)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
