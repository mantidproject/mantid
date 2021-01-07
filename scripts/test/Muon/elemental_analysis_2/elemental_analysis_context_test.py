# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.Common.muon_load_data import MuonLoadData


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
        new_run_object = mock.Mock(run_number=1234)
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)
        self.assertEqual(self.context.run_info[0].run_number, 1234)

    def test_clear_run_info(self):
        new_run_object = mock.Mock()
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)

        self.context.clear_run_info()
        self.assertEqual(self.context.run_info, [])


class EAGroupContextTest(unittest.TestCase):
    def setUp(self):
        self.context = EAGroupContext()
        self.loadedData = MuonLoadData()

    def create_group_workspace_and_load(self):
        grpws = WorkspaceGroup()
        ws_detector1 = '9999; Detector 1'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = '9999; Detector 2'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        run = 9999
        self.loadedData.add_data(run=[run], workspace=grpws)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_add_group(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        empty_group = []
        new_group = self.context.add_new_group(empty_group, self.loadedData)
        self.assertEqual(len(new_group), 2)

    def test_remove_group(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)
        group_name1 = '9999; Detector 1'
        self.assertTrue(group_name1 in self.context.group_names)

        self.context.remove_group(group_name1)
        self.assertFalse(group_name1 in self.context.group_names)

    def test_reset_group_to_default(self):
        self.loadedData.clear()
        self.assertEqual(self.loadedData.num_items(),0)
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)

    def test_clear(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)

        self.context.clear()
        self.assertEqual(len(self.context.groups), 0)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
