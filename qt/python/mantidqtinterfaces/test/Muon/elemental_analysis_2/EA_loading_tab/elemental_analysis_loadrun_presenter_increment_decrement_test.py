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
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class LoadRunWidgetIncrementDecrementSingleFileModeTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model and thread_model.worker:
            while thread_model.worker.is_alive():
                time.sleep(0.1)
            QApplication.sendPostedEvents()

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_ea_tests(self)

        self.view = mock.Mock()
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)
        self.presenter = LoadRunWidgetPresenterEA(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.model.execute = mock.Mock()

        self.load_single_run()

    def tearDown(self):
        self.obj = None

    def load_single_run(self):
        self.model._data_context.current_runs = []
        self._loaded_run = 5647

        self.view.get_run_edit_text.return_value = str(self._loaded_run)

        self.presenter.handle_run_changed_by_user()

        grpws = mock.Mock()
        for run in self.model._runs:
            self.model._loaded_data_store.add_data(run=[run], workspace=grpws)
            self.model._data_context.current_runs.append(run)

        self.wait_for_thread(self.presenter._load_thread)

    @staticmethod
    def load_failure(self):
        raise ValueError("Error text")

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Test the increment/decrement buttons
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_decrement_run_attempts_to_load_the_correct_run(self):
        original_run = self._loaded_run
        grpws = mock.Mock()

        self.presenter.handle_decrement_run()
        for run in self.model._runs:
            self.model._loaded_data_store.add_data(run=[run], workspace=grpws)

        self.wait_for_thread(self.presenter._load_thread)
        self.view.get_run_edit_text.return_value = str(self.presenter.runs[-1][0])

        self.assertEqual(self.presenter.runs[-1], [original_run - 1])

        run_in_view = self.view.get_run_edit_text()
        self.assertEqual(run_in_view, str(original_run - 1))

    def test_that_increment_run_attempts_to_load_the_correct_run(self):
        original_run = self._loaded_run
        grpws = mock.Mock()

        self.presenter.handle_increment_run()
        for run in self.model._runs:
            self.model._loaded_data_store.add_data(run=[run], workspace=grpws)

        self.view.get_run_edit_text.return_value = str(self.presenter.runs[-1][0])

        self.wait_for_thread(self.presenter._load_thread)
        self.assertEqual(self.presenter.runs[-1], [original_run + 1])

        run_in_view = self.view.get_run_edit_text()
        self.assertEqual(run_in_view, str(original_run + 1))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
