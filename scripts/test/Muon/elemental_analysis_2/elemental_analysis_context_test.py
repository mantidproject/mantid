# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from unittest import mock
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace,CreateWorkspace
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws,remove_ws_if_present


class ElementalAnalysisContextTest(unittest.TestCase):
    def setUp(self):
        self.context = ElementalAnalysisContext(data_context=DataContext(),ea_group_context=EAGroupContext())

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_name(self):
        self.assertEqual(self.context.name, "Elemental Analysis 2")

    @mock.patch('Muon.GUI.ElementalAnalysis2.context.context.retrieve_ws')
    @mock.patch('Muon.GUI.ElementalAnalysis2.context.context.remove_ws_if_present')
    @mock.patch('Muon.GUI.ElementalAnalysis2.context.ea_group_context.EAGroupContext.__getitem__')
    def test_rebin(self,mock_get_item,mock_remove_ws,mock_retrieve_ws):

        mock_get_item.return_value = EAGroup("9999; Detector 1","detector 1" , "9999")
        name = '9999; Detector 1'
        rebinned_name = '9999; Detector 1' + "_EA_Rebinned_Variable"
        mock_params = [0,2,9]

        dataX = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        dataY = [1, 1, 1, 1, 1, 1, 1, 1, 1]

        CreateWorkspace(OutputWorkspace=name, DataX=dataX, DataY=dataY)
        self.context._run_rebin("9999; Detector 1","Variable",mock_params)

        correct_data = CreateWorkspace(OutputWorkspace = "correct_data",DataX = [0,2,4,6,8,9],DataY = [2,2,2,2,1])

        #Assert Statements
        self.assert_workspace_equal(correct_data,retrieve_ws(rebinned_name))
        mock_remove_ws.assert_has_calls([mock.call(rebinned_name)])
        mock_retrieve_ws.assert_has_calls([mock.call("9999")])
        mock_get_item.assert_has_calls([mock.call(name)])

        #clean up
        remove_ws_if_present(name)
        remove_ws_if_present("correct_data")
        remove_ws_if_present(rebinned_name)

    def test_remove_workspace(self):
        pass

    def assert_workspace_equal(self,workspace1,workspace2):
        self.assertEqual(workspace1.getNumberHistograms() , workspace2.getNumberHistograms())
        for i in range(workspace1.getNumberHistograms()):
            print(workspace2.readY(i))
            self.assertTrue(np.array_equal(workspace1.readX(i), workspace2.readX(i)))
            self.assertTrue(np.array_equal(workspace1.readY(i), workspace2.readY(i)))
            self.assertTrue(np.array_equal(workspace1.readE(i), workspace2.readE(i)))


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
