# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FrameworkManager

from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class CorrectionsModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.model = CorrectionsModel(context.data_context, context.corrections_context)
        self.runs = [[84447], [84448], [84449]]
        self.coadd_runs = [[84447, 84448, 84449]]

    def _setup_for_multiple_runs(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def _setup_for_coadd_mode(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.coadd_runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def tearDown(self):
        self.model = None
        AnalysisDataService.clear()

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_not_in_coadd_mode(self):
        self._setup_for_multiple_runs()
        self.assertEqual(self.model.number_of_run_strings, len(self.runs))

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.number_of_run_strings, len(self.coadd_runs))

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_not_in_coadd_mode(self):
        self._setup_for_multiple_runs()
        self.assertEqual(self.model.run_number_strings(), ["84447", "84448", "84449"])

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.run_number_strings(), ["84447-84449"])

    def test_that_current_runs_returns_none_when_a_run_string_is_not_selected(self):
        self.assertEqual(self.model.current_runs(), None)

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_non_coadd_mode(self):
        self._setup_for_multiple_runs()

        self.model.set_current_run_string("84448")

        self.assertEqual(self.model.current_runs(), [84448])

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_coadd_mode(self):
        self._setup_for_coadd_mode()

        self.model.set_current_run_string("84447-84449")

        self.assertEqual(self.model.current_runs(), [84447, 84448, 84449])


if __name__ == '__main__':
    unittest.main()
