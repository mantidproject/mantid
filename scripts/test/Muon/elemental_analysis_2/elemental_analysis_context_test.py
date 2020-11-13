# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.context.context import DataContext, ElementalAnalysisContext


class ElementalAnalysisContextTest(unittest.TestCase):
    def setUp(self):
        self.context = ElementalAnalysisContext()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_name(self):
        self.assertEqual(self.context.name, "Elemental Analysis 2")


class DataContextTest(unittest.TestCase):
    def setUp(self):
        self.context = DataContext()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_initialised_empty(self):
        self.assertEqual(self.context.current_runs, [])
        self.assertEqual(self.context.run_info, [])

    def test_add_run_object_to_run_info(self):
        new_runObject = mock.Mock()
        self.context.run_info_update(new_runObject)
        self.assertTrue(len(self.context.run_info) == 1)

    def test_clear_run_info(self):
        new_runObject = mock.Mock()
        self.context.run_info_update(new_runObject)
        self.assertTrue(len(self.context.run_info) == 1)

        self.context.clear_run_info()
        self.assertEqual(self.context.run_info, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
