# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QApplication, QWidget

from Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class LoadRunWidgetPresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            QApplication.sendPostedEvents()

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_ea_tests(self)

        self.view = mock.create_autospec(LoadRunWidgetView, autospec=True)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
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

        self.presenter.handle_run_changed_by_user()

        grpws = mock.Mock()
        for run in self.model._runs:
            self.model._loaded_data_store.add_data(run=[run], workspace=grpws)

        self.wait_for_thread(self.presenter._load_thread)

        self.assertEqual(self.presenter.runs, [[5555]])

    def test_clear_loaded_data(self):
        """
            Tests that clearing loaded data clears both
            the loaded_data in the model and the view
        """
        run = 1265
        self.view.get_run_edit_text.return_value = str(run)

        grpws = mock.Mock()
        self.model._loaded_data_store.add_data(run=[run], workspace=grpws)

        self.assertTrue(len(self.model.loaded_runs) > 0)
        self.assertEqual(self.view.get_run_edit_text(), str(run))

        self.presenter.clear_loaded_data()

        self.assertTrue(len(self.model.loaded_runs) == 0)
        self.assertEqual(self.view.clear.call_count, 1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
