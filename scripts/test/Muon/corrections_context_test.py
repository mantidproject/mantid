# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext
from Muon.GUI.Common.muon_load_data import MuonLoadData


class CorrectionsContextTest(unittest.TestCase):

    def setUp(self):
        self.muon_data = MuonLoadData()
        self.corrections_context = CorrectionsContext(self.muon_data)

    def test_that_the_context_has_been_instantiated_with_the_expected_context_data(self):
        self.assertEqual(self.corrections_context.current_run_string, None)
        self.assertEqual(self.corrections_context.dead_time_source, "FromFile")
        self.assertEqual(self.corrections_context.dead_time_table_name_from_ads, None)

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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
