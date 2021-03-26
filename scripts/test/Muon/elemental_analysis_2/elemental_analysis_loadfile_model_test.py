# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.ElementalAnalysis2.load_widget.load_models import BrowseFileWidgetModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests
from Muon.GUI.Common.muon_load_data import MuonLoadData


@start_qapplication
class LoadFileWidgetModelEATest(unittest.TestCase):
    def setUp(self):
        setup_context_for_tests(self)
        self.model = BrowseFileWidgetModel(self.loaded_data, self.context)
        self.model._loaded_data_store = MuonLoadData()

    def assert_model_empty(self):
        self.assertEqual(self.model.loaded_runs, [])
        self.assertEqual(self.model.current_runs, [])

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_initialized_with_empty_variables(self):
        self.assert_model_empty()

    def test_model_is_cleared_correctly(self):
        self.model._loaded_data_store.add_data(run=1234)
        self.assertEqual(self.model.loaded_runs, [1234])
        self.model.clear()
        self.assert_model_empty()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
