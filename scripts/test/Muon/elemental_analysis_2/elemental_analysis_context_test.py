# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext


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
        self.assertEquals(self.context.current_runs, [])
        self.assertEquals(self.context.run_info, [])

    def test_add_run_object_to_run_info(self):
        new_run_object = mock.Mock(run_number=1234)
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)
        self.assertEquals(self.context.run_info[0].run_number, 1234)

    def test_clear_run_info(self):
        new_run_object = mock.Mock()
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)

        self.context.clear_run_info()
        self.assertEquals(self.context.run_info, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
