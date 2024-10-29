# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FrameworkManager
from mantid.simpleapi import Load

from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_model import DeadTimeCorrectionsModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


class DeadTimeCorrectionsModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        context = setup_context()
        self.corrections_model = CorrectionsModel(context)
        self.model = DeadTimeCorrectionsModel(self.corrections_model, context.data_context, context.corrections_context)
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

    def test_that_set_dead_time_source_to_from_file_will_set_the_expected_data_in_the_context(self):
        self.model.set_dead_time_source_to_from_file()
        self.assertEqual(self.model._corrections_context.dead_time_source, "FromFile")
        self.assertEqual(self.model._corrections_context.dead_time_table_name_from_ads, None)
        self.assertTrue(self.model.is_dead_time_source_from_data_file())

    def test_that_set_dead_time_source_to_from_ads_will_set_the_expected_data_in_the_context(self):
        table_name = "Table name"
        self.model.set_dead_time_source_to_from_ads(table_name)

        self.assertEqual(self.model._corrections_context.dead_time_source, "FromADS")
        self.assertEqual(self.model._corrections_context.dead_time_table_name_from_ads, table_name)
        self.assertTrue(self.model.is_dead_time_source_from_workspace())

    def test_that_set_dead_time_source_to_from_ads_will_set_the_expected_data_in_the_context_to_none(self):
        self.model.set_dead_time_source_to_from_ads("None")

        self.assertEqual(self.model._corrections_context.dead_time_source, None)
        self.assertEqual(self.model._corrections_context.dead_time_table_name_from_ads, None)
        self.assertTrue(self.model.is_dead_time_source_from_none())

    def test_that_set_dead_time_source_to_none_will_set_the_expected_data_in_the_context_to_none(self):
        self.model.set_dead_time_source_to_from_ads("Table name")
        self.assertTrue(self.model._corrections_context.dead_time_source is not None)
        self.assertTrue(self.model._corrections_context.dead_time_table_name_from_ads is not None)

        self.model.set_dead_time_source_to_none()

        self.assertEqual(self.model._corrections_context.dead_time_source, None)
        self.assertEqual(self.model._corrections_context.dead_time_table_name_from_ads, None)
        self.assertTrue(self.model.is_dead_time_source_from_none())

    def test_that_dead_times_average_will_return_the_expected_dead_time_average_and_range(self):
        dead_time_table_name = "dead_time_table"
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.model.set_dead_time_source_to_from_ads(dead_time_table_name)

        self.assertAlmostEqual(self.model.dead_times_average(), 0.010244, 6)

        dead_time_range = self.model.dead_times_range()
        self.assertAlmostEqual(dead_time_range[0], 0.000035, 6)
        self.assertAlmostEqual(dead_time_range[1], 0.020642, 6)

    def test_that_validate_selected_dead_time_workspace_will_return_an_empty_string_for_valid_data(self):
        self.model._data_context.current_workspace.getNumberHistograms = mock.Mock(return_value=64)

        dead_time_table_name = "dead_time_table"
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.assertEqual(self.model.validate_selected_dead_time_workspace(dead_time_table_name), "")

    def test_that_validate_selected_dead_time_workspace_returns_an_error_message_when_the_num_of_histograms_is_invalid(self):
        self.model._data_context.current_workspace.getNumberHistograms = mock.Mock(return_value=62)

        dead_time_table_name = "dead_time_table"
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="output_ws", DeadTimeTable=dead_time_table_name)

        self.assertEqual(
            self.model.validate_selected_dead_time_workspace(dead_time_table_name),
            "The number of histograms (62) does not match the number of rows (64) in dead time table.",
        )

    def test_that_validate_selected_dead_time_workspace_will_return_an_error_when_the_workspace_is_not_in_the_ADS(self):
        dead_time_table_name = "dead_time_table"
        self.assertEqual(
            self.model.validate_selected_dead_time_workspace(dead_time_table_name), "Workspace 'dead_time_table' does not exist in the ADS."
        )


if __name__ == "__main__":
    unittest.main()
