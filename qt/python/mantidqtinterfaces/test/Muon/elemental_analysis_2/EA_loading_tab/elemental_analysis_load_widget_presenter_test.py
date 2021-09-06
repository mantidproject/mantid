# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from mantidqtinterfaces.Muon.GUI.Common.load_file_widget.view import BrowseFileWidgetView
from mantidqtinterfaces.Muon.GUI.Common.load_run_widget.load_run_view import LoadRunWidgetView
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadWidgetModel, LoadRunWidgetModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_widget_presenter import LoadWidgetPresenterEA
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_run_widget_presenter import LoadRunWidgetPresenterEA
from mantidqtinterfaces.Muon.GUI.MuonAnalysis.load_widget.load_widget_view import LoadWidgetView


@start_qapplication
class LoadWidgetPresenterTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_ea_tests(self)
        self.load_file_view = BrowseFileWidgetView(self.obj)
        self.load_run_view = LoadRunWidgetView(self.obj)
        self.load_run_view.set_current_instrument = mock.Mock()
        self.view = LoadWidgetView(parent=self.obj,
                                   load_file_view=self.load_file_view,
                                   load_run_view=self.load_run_view)
        self.model = LoadWidgetModel(self.loaded_data, self.context)
        self.model.instrument = mock.Mock()
        self.presenter = LoadWidgetPresenterEA(self.view, self.model)
        loaded_data = mock.Mock()
        context = mock.Mock()
        self.run_widget = LoadRunWidgetPresenterEA(self.load_run_view, LoadRunWidgetModel(loaded_data, context))
        self.presenter.set_load_run_widget(self.run_widget)

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_initialized_to_contain_no_data(self):
        self.assertEqual(self.model.runs, [])

    def test_clear_data(self):
        self.model.current_runs = mock.Mock()
        self.assertTrue(self.model.runs)
        self.presenter.clear_data()
        self.assertEqual(self.model.runs, [])

    def test_clear_data_and_view(self):
        self.load_run_view.set_run_edit_text("1234-5")
        self.model.current_runs = [[1234], [1235]]
        self.assertEqual(len(self.model.runs), 2)
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1234-5")
        self.presenter.clear_data_and_view()
        self.assertEqual(self.model.runs, [])
        self.assertEqual(self.load_run_view.get_run_edit_text(), "")

    def test_update_view_from_model(self):
        self.assertEqual(self.load_run_view.get_run_edit_text(), "")
        self.model.current_runs = [[1234]]
        self.presenter.update_view_from_model()
        self.assertEqual(self.load_run_view.get_run_edit_text(), "1234")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
