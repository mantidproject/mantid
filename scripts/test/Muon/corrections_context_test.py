# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext
from Muon.GUI.Common.corrections_tab_widget.background_corrections_model import BackgroundCorrectionData
from Muon.GUI.Common.muon_load_data import MuonLoadData


class CorrectionsContextTest(unittest.TestCase):

    def setUp(self):
        self.muon_data = MuonLoadData()
        self.corrections_context = CorrectionsContext(self.muon_data)

    def test_that_the_context_has_been_instantiated_with_the_expected_context_data(self):
        self.assertEqual(self.corrections_context.current_run_string, None)

        self.assertEqual(self.corrections_context.dead_time_source, "FromFile")
        self.assertEqual(self.corrections_context.dead_time_table_name_from_ads, None)

        self.assertEqual(self.corrections_context.background_corrections_mode, "None")
        self.assertEqual(self.corrections_context.selected_function, "Flat Background + Exp Decay")
        self.assertEqual(self.corrections_context.selected_group, "All")
        self.assertEqual(self.corrections_context.show_all_runs, False)

    def test_that_the_current_run_string_can_be_set_as_expected(self):
        run_string = "62260"
        self.corrections_context.current_run_string = run_string
        self.assertEqual(self.corrections_context.current_run_string, run_string)

    def test_that_the_dead_time_source_can_be_set_as_expected(self):
        source = "FromADS"
        self.corrections_context.dead_time_source = source
        self.assertEqual(self.corrections_context.dead_time_source, source)

    def test_that_the_dead_time_table_name_from_ads_can_be_set_as_expected(self):
        table_name = "MUSR62260 dead time table"
        self.corrections_context.dead_time_table_name_from_ads = table_name
        self.assertEqual(self.corrections_context.dead_time_table_name_from_ads, table_name)

    def test_that_the_dead_time_table_name_from_file_can_be_set_as_expected(self):
        table_name = "MUSR62260 dead time table"
        self.corrections_context.dead_time_table_name_from_file = table_name
        self.assertEqual(self.corrections_context.dead_time_table_name_from_file, table_name)

    def test_that_current_dead_time_table_name_returns_the_expected_table_when_the_source_is_from_file(self):
        table_from_file = "MUSR62260 dead time table"
        table_from_ads = "MUSR62265 dead time table"

        self.corrections_context.get_default_dead_time_table_name_for_run = mock.Mock(return_value=table_from_file)

        self.corrections_context.dead_time_source = "FromFile"
        self.corrections_context.dead_time_table_name_from_ads = table_from_ads

        self.assertEqual(self.corrections_context.current_dead_time_table_name_for_run("MUSR", [62260]),
                         table_from_file)

    def test_that_current_dead_time_table_name_returns_the_expected_table_when_the_source_is_from_ads(self):
        table_from_ads = "MUSR62265 dead time table"

        self.corrections_context.dead_time_source = "FromADS"
        self.corrections_context.dead_time_table_name_from_ads = table_from_ads

        self.assertEqual(self.corrections_context.current_dead_time_table_name_for_run("MUSR", [62265]), table_from_ads)

    def test_that_current_dead_time_table_name_returns_none_when_the_source_is_from_none(self):
        table_from_ads = "MUSR62265 dead time table"

        self.corrections_context.dead_time_source = None
        self.corrections_context.dead_time_table_name_from_ads = table_from_ads

        self.assertEqual(self.corrections_context.current_dead_time_table_name_for_run("MUSR", [62265]), None)

    def test_that_the_background_corrections_mode_can_be_set_as_expected(self):
        background_corrections_mode = "Auto"
        self.corrections_context.background_corrections_mode = background_corrections_mode
        self.assertEqual(self.corrections_context.background_corrections_mode, background_corrections_mode)

    def test_that_the_selected_function_can_be_set_as_expected(self):
        selected_function = "Flat Background + Exp Decay"
        self.corrections_context.selected_function = selected_function
        self.assertEqual(self.corrections_context.selected_function, selected_function)

    def test_that_the_selected_group_can_be_set_as_expected(self):
        selected_group = "fwd"
        self.corrections_context.selected_group = selected_group
        self.assertEqual(self.corrections_context.selected_group, selected_group)

    def test_that_show_all_runs_can_be_set_as_expected(self):
        show_all_runs = True
        self.corrections_context.show_all_runs = show_all_runs
        self.assertEqual(self.corrections_context.show_all_runs, show_all_runs)

    def test_that_the_background_correction_data_can_be_set_as_expected(self):
        run_group = tuple(["84447", "fwd"])
        start_x, end_x = 15.0, 30.0
        self.corrections_context.background_correction_data[run_group] = BackgroundCorrectionData(True, 5, start_x,
                                                                                                  end_x)

        self.assertTrue(run_group in self.corrections_context.background_correction_data)
        self.assertEqual(self.corrections_context.background_correction_data[run_group].use_raw, True)
        self.assertEqual(self.corrections_context.background_correction_data[run_group].rebin_fixed_step, 5)
        self.assertEqual(self.corrections_context.background_correction_data[run_group].start_x, start_x)
        self.assertEqual(self.corrections_context.background_correction_data[run_group].end_x, end_x)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
