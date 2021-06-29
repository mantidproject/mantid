# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext


class CorrectionsContextTest(unittest.TestCase):

    def setUp(self):
        self.corrections_context = CorrectionsContext()

    def test_that_the_context_has_been_instantiated_with_the_expected_context_data(self):
        self.assertEqual(self.corrections_context.current_run_string, None)
        self.assertEqual(self.corrections_context.dead_time_source, None)
        self.assertEqual(self.corrections_context.dead_time_table_name, None)

    def test_that_the_current_run_string_can_be_set_as_expected(self):
        run_string = "62260"
        self.corrections_context.current_run_string = run_string
        self.assertEqual(self.corrections_context.current_run_string, run_string)

    def test_that_the_dead_time_source_can_be_set_as_expected(self):
        source = "FromADS"
        self.corrections_context.dead_time_source = source
        self.assertEqual(self.corrections_context.dead_time_source, source)

    def test_that_the_dead_time_table_name_can_be_set_as_expected(self):
        table_name = "MUSR62260 dead time table"
        self.corrections_context.dead_time_table_name = table_name
        self.assertEqual(self.corrections_context.dead_time_table_name, table_name)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
