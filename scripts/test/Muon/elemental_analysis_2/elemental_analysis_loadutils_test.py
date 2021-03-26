# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import Muon.GUI.ElementalAnalysis2.load_widget.load_utils_ea as load_utils_ea
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadRunWidgetModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests
from Muon.GUI.ElementalAnalysis2.context.context import RunObject
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace


class EAFileUtilsTest(unittest.TestCase):
    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)

    def create_run_workspaces(self, run):
        detectors = ['Detector 1', 'Detector 2', 'Detector 3', 'Detector 4']
        grpws = WorkspaceGroup()
        ws_detector1 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector1)
        ws_detector2 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector2)
        ws_detector3 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector3)
        ws_detector4 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector4)
        run_results = RunObject(run, detectors, grpws)

        self.model._loaded_data_store.add_data(run=[run], workspace=grpws)
        self.model._data_context.run_info_update(run_results)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_check_all_detectors_complete_true(self):
        run_detectors = ['Detector 1', 'Detector 3', 'Detector 5']
        finished_detectors = []
        result = load_utils_ea.check_for_unused_detectors(run_detectors, finished_detectors)
        self.assertTrue(result)

    def test_check_all_detectors_complete_false(self):
        run_detectors = ['Detector 1', 'Detector 4']
        finished_detectors = ['Detector 1', 'Detector 4']
        result = load_utils_ea.check_for_unused_detectors(run_detectors, finished_detectors)
        self.assertFalse(result)

    def test_get_detectors(self):
        run = 1234
        detectors = ['Detector 1', 'Detector 2', 'Detector 3', 'Detector 4']
        groupws = mock.Mock()
        run_results = RunObject(run, detectors, groupws)
        self.model._data_context.run_info_update(run_results)
        list_detectors = load_utils_ea.get_detectors(self.model, run)
        self.assertEqual(list_detectors, detectors)

    def test_find_ws_to_use(self):
        run = 5555
        detector = 'Detector 3'
        run_detectors = ['Detector 1', 'Detector 2', 'Detector 3', 'Detector 4']

        grpws = WorkspaceGroup()
        ws_detector1 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector1)
        ws_detector2 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector2)
        ws_detector3 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector3)
        ws_detector4 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector4)

        self.model._loaded_data_store.add_data(run=[run], workspace=grpws)

        ws = load_utils_ea.find_ws_to_use(self.model, run_detectors, detector, run)
        self.assertEqual(ws.name(), 'ws_detector3')

    def test_add_detector_workspace_to_group(self):
        """
            The test will fail if it cannot delete a workspace because it does not exist
        """
        grp_ws = WorkspaceGroup()
        new_ws = CreateSampleWorkspace()
        new_ws_name = 'Test Workspace Name'
        detector = 'Detector 3'
        finished_detectors = []
        ws_list = []
        load_utils_ea.add_detector_workspace_to_group(grp_ws, new_ws, new_ws_name, detector, finished_detectors,
                                                      ws_list)
        self.assertEqual(ws_list, ['Test Workspace Name'])
        self.assertEqual(finished_detectors, ['Detector 3'])
        self.assertEqual(grp_ws.getNames()[0], 'Test Workspace Name')

    def test_finalise_groupworkspace(self):
        """
            The test will fail if it cannot delete a workspace because it does not exist
        """
        grpws = WorkspaceGroup()
        ws_detector1 = CreateSampleWorkspace()
        grpws.addWorkspace(ws_detector1)
        grpws_name = 'New Group Workspace'
        ws1 = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        ws3 = CreateSampleWorkspace()
        ws_list = [ws1, ws2, ws3]
        load_utils_ea.finalise_groupworkspace(grpws, grpws_name, ws_list)
        self.assertTrue(AnalysisDataService.doesExist('New Group Workspace'))

    def test_combine_loaded_runs(self):
        """
            As a result of this function there should be:-
                - a groupworkspace named after the range of runs used e.g. '1234-1237'
                - a workspace for each detector named [detector]_[groupworkspace]
                    e.g. 'Detector 1_1234-1237'
        """
        self.assertEqual(self.model._loaded_data_store.num_items(), 0)
        run_list = [1234, 1235, 1236, 1237]
        for run in run_list:
            self.create_run_workspaces(run)
        self.assertEqual(self.model._loaded_data_store.num_items(), 4)

        load_utils_ea.combine_loaded_runs(self.model, run_list)
        self.assertTrue('1234-1237' in AnalysisDataService.getObjectNames())
        self.assertEqual(AnalysisDataService.retrieve('1234-1237').size(), 4)
        self.assertTrue('Detector 2_1234-1237' in AnalysisDataService.getObjectNames())


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
