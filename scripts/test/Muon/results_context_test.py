# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FrameworkManager, WorkspaceFactory
from Muon.GUI.Common.ADSHandler.ADS_calls import add_ws_to_ads
from Muon.GUI.Common.contexts.results_context import ResultsContext


class ResultsContextTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.results_context = ResultsContext()
        self.result_table_names = ["Name1", "Name2"]

        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        add_ws_to_ads("Name1", workspace)
        add_ws_to_ads("Name2", workspace)

    def test_that_the_context_has_been_instantiated_with_empty_data(self):
        self.assertEqual(self.results_context.result_table_names, [])

    def test_that_the_result_table_names_can_be_set_in_the_context(self):
        self.results_context.result_table_names = self.result_table_names
        self.assertEqual(self.results_context.result_table_names, self.result_table_names)

    def test_that_add_result_table_will_add_a_table_name(self):
        self.results_context.add_result_table(self.result_table_names[0])
        self.assertEqual(self.results_context.result_table_names, [self.result_table_names[0]])

        self.results_context.add_result_table(self.result_table_names[1])
        self.assertEqual(self.results_context.result_table_names, self.result_table_names)

    def test_that_add_result_table_will_not_add_a_duplicate_table_name(self):
        self.results_context.add_result_table(self.result_table_names[0])
        self.assertEqual(self.results_context.result_table_names, [self.result_table_names[0]])

        self.results_context.add_result_table(self.result_table_names[0])
        self.assertEqual(self.results_context.result_table_names, [self.result_table_names[0]])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
