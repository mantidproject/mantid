# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FrameworkManager
from mantid.simpleapi import LoadMuonNexus

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

    def _setup_for_non_coadd_mode(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def _setup_for_coadd_mode(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.coadd_runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def tearDown(self):
        self.model = None
        AnalysisDataService.clear()

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_not_in_coadd_mode(self):
        self._setup_for_non_coadd_mode()
        self.assertEqual(self.model.number_of_run_strings, 3)

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.number_of_run_strings, 1)

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_not_in_coadd_mode(self):
        self._setup_for_non_coadd_mode()
        self.assertEqual(self.model.run_number_strings(), ["84447", "84448", "84449"])

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.run_number_strings(), ["84447-84449"])

    def test_that_current_runs_returns_none_when_a_run_string_is_not_selected(self):
        self.assertEqual(self.model.current_runs(), None)

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_non_coadd_mode(self):
        self._setup_for_non_coadd_mode()

        self.model.set_current_run_string("84448")

        self.assertEqual(self.model.current_runs(), [84448])

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_coadd_mode(self):
        self._setup_for_coadd_mode()

        self.model.set_current_run_string("84447-84449")

        self.assertEqual(self.model.current_runs(), [84447, 84448, 84449])

    def test_that_set_dead_time_source_to_from_file_will_set_the_expected_data_in_the_context(self):
        self.model.set_dead_time_source_to_from_file()
        self.assertEqual(self.model._corrections_context.dead_time_source, "FromFile")
        self.assertEqual(self.model._corrections_context.dead_time_table_name, None)
        self.assertTrue(self.model.is_dead_time_source_from_data_file())

    def test_that_set_dead_time_source_to_from_ads_will_set_the_expected_data_in_the_context(self):
        table_name = "Table name"
        self.model.set_dead_time_source_to_from_ads(table_name)

        self.assertEqual(self.model._corrections_context.dead_time_source, "FromADS")
        self.assertEqual(self.model._corrections_context.dead_time_table_name, table_name)
        self.assertTrue(self.model.is_dead_time_source_from_workspace())

    def test_that_set_dead_time_source_to_from_ads_will_set_the_expected_data_in_the_context_to_none(self):
        self.model.set_dead_time_source_to_from_ads("None")

        self.assertEqual(self.model._corrections_context.dead_time_source, None)
        self.assertEqual(self.model._corrections_context.dead_time_table_name, None)
        self.assertTrue(self.model.is_dead_time_source_from_none())

    def test_that_set_dead_time_source_to_none_will_set_the_expected_data_in_the_context_to_none(self):
        self.model.set_dead_time_source_to_from_ads("Table name")
        self.assertTrue(self.model._corrections_context.dead_time_source is not None)
        self.assertTrue(self.model._corrections_context.dead_time_table_name is not None)

        self.model.set_dead_time_source_to_none()

        self.assertEqual(self.model._corrections_context.dead_time_source, None)
        self.assertEqual(self.model._corrections_context.dead_time_table_name, None)
        self.assertTrue(self.model.is_dead_time_source_from_none())

    def test_that_dead_times_average_will_return_the_expected_dead_time_average(self):
        dead_time_table_name = "dead_time_table"
        LoadMuonNexus(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.model.set_dead_time_source_to_from_ads(dead_time_table_name)

        self.assertAlmostEqual(self.model.dead_times_average(), 0.010244, 6)

    def test_that_dead_times_range_will_return_the_expected_dead_time_range(self):
        dead_time_table_name = "dead_time_table"
        LoadMuonNexus(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.model.set_dead_time_source_to_from_ads(dead_time_table_name)

        dead_time_range = self.model.dead_times_range()
        self.assertAlmostEqual(dead_time_range[0], 0.000035, 6)
        self.assertAlmostEqual(dead_time_range[1], 0.020642, 6)

    def test_that_validate_selected_dead_time_workspace_will_return_an_empty_string_for_valid_data(self):
        self.model._data_context.current_workspace.getNumberHistograms = mock.Mock(return_value=64)

        dead_time_table_name = "dead_time_table"
        LoadMuonNexus(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.assertEqual(self.model.validate_selected_dead_time_workspace(dead_time_table_name), "")

    def test_that_validate_selected_dead_time_workspace_returns_an_error_message_when_the_num_of_histograms_is_invalid(self):
        self.model._data_context.current_workspace.getNumberHistograms = mock.Mock(return_value=62)

        dead_time_table_name = "dead_time_table"
        LoadMuonNexus(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.assertEqual(self.model.validate_selected_dead_time_workspace(dead_time_table_name),
                         "The number of histograms does not match the number of rows in dead time table (62 != 64).")

    def test_that_validate_selected_dead_time_workspace_will_return_an_error_when_the_workspace_is_not_in_the_ADS(self):
        dead_time_table_name = "dead_time_table"
        self.assertEqual(self.model.validate_selected_dead_time_workspace(dead_time_table_name),
                         "Workspace 'dead_time_table' does not exist in the ADS.")


if __name__ == '__main__':
    unittest.main()
