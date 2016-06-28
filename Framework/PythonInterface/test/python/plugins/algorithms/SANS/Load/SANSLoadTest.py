import unittest
import mantid

from mantid.dataobjects import (Workspace2D, EventWorkspace)
from mantid.api import (AnalysisDataService, AlgorithmManager, WorkspaceGroup)

from SANS.Load.SANSLoadData import SANSLoadDataFactory
from SANS2.State.SANSStateData import (SANSStateDataISIS, SANSDataType)
from SANS2.State.SANSState import SANSStateISIS


def remove_all_workspaces_from_ads():
    workspaces_on_the_ads = AnalysisDataService.getObjectNames()
    for name in workspaces_on_the_ads:
        AnalysisDataService.remove(name)


def compare_workspaces(workspace1, workspace2):
    try:
        alg = AlgorithmManager.createUnmanaged("CompareWorkspaces")
        alg.initialize()
        alg.setChild(True)
        alg.setRethrows(True)
        alg.setProperty("Workspace1", workspace1)
        alg.setProperty("Workspace2", workspace2)
        alg.setProperty("Tolerance", 1e-6)
        alg.setProperty("ToleranceRelErr", True)
        alg.setProperty("CheckAllData", True)
        alg.execute()
    except RuntimeError:
        raise RuntimeError("Comparison was wrong.")


class SANSLoadFactoryTest(unittest.TestCase):
    def test_that_valid_file_information_does_not_raise(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data = SANSStateDataISIS()
        ws_name_sample = "SANS2D00022024"
        data.sample_scatter = ws_name_sample
        state = SANSStateISIS()
        state.data = data

        # Act + Assert
        try:
            load_factory.create_loader(state)
            did_not_raise = True
        except NotImplementedError:
            did_not_raise = True
        self.assertTrue(did_not_raise)


class SANSLoaderTest(unittest.TestCase):
    def test_that_can_load_isis_nexus_file_with_histo_data_and_single_period(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00000808"
        data_info.sample_scatter = ws_name_sample
        ws_name_sample_transmission = "SANS2D00028784"
        data_info.sample_transmission = ws_name_sample_transmission
        ws_name_sample_direct = "SANS2D00028804"
        data_info.sample_direct = ws_name_sample_direct
        state = SANSStateISIS()
        state.data = data_info

        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=False)

        # Assert
        self.assertTrue(len(workspace) == 3)
        self.assertTrue(len(workspace_monitors) == 1)

        # Check the monitor workspaces -- we should only have the one of the sample scatter
        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        # Check the actual workspaces
        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], Workspace2D))
        self.assertTrue(SANSDataType.SampleTransmission in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleTransmission][0], Workspace2D))
        self.assertTrue(SANSDataType.SampleDirect in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleDirect][0], Workspace2D))

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 0)

    def test_that_can_load_raw_file_with_histo_data_and_single_period(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00000808.raw"
        data_info.sample_scatter = ws_name_sample
        ws_name_sample_transmission = "SANS2D00028784"
        data_info.sample_transmission = ws_name_sample_transmission
        ws_name_sample_direct = "SANS2D00028804"
        data_info.sample_direct = ws_name_sample_direct
        state = SANSStateISIS()
        state.data = data_info

        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=False)

        # Assert
        self.assertTrue(len(workspace) == 3)
        self.assertTrue(len(workspace_monitors) == 1)

        # Check the monitor workspaces -- we should only have the one of the sample scatter
        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        # Check the actual workspaces
        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], Workspace2D))
        self.assertTrue(SANSDataType.SampleTransmission in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleTransmission][0], Workspace2D))
        self.assertTrue(SANSDataType.SampleDirect in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleDirect][0], Workspace2D))

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 0)

    def test_that_can_load_isis_nexus_file_with_histo_data_and_multi_period(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00005512.nxs"
        data_info.sample_scatter = ws_name_sample
        state = SANSStateISIS()
        state.data = data_info
        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=False)

        # Assert
        self.assertTrue(len(workspace) == 1)
        self.assertTrue(len(workspace_monitors) == 1)

        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], Workspace2D))

        self.assertTrue(len(workspace[SANSDataType.SampleScatter]) ==
                        len(workspace_monitors[SANSDataType.SampleScatter]))
        self.assertTrue(len(workspace[SANSDataType.SampleScatter]) == 13)

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 0)

    def test_that_can_load_isis_nexus_file_with_histo_data_and_multi_period_and_select_single_period(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00005512.nxs"
        data_info.sample_scatter = ws_name_sample
        data_info.sample_scatter_period = 3
        state = SANSStateISIS()
        state.data = data_info
        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=False)

        # Assert
        self.assertTrue(len(workspace) == 1)
        self.assertTrue(len(workspace_monitors) == 1)

        # We get a Workspace2D instead of a WorkspaceGroup
        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        # We get a Workspace2D instead of a WorkspaceGroup
        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], Workspace2D))

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 0)

    def test_that_can_load_isis_nexus_file_with_histo_data_and_multi_period_and_add_to_ads(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00005512.nxs"
        data_info.sample_scatter = ws_name_sample
        state = SANSStateISIS()
        state.data = data_info
        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=True)

        # Assert
        self.assertTrue(len(workspace) == 1)
        self.assertTrue(len(workspace_monitors) == 1)

        # We get a Workspace2D
        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        # We get a Workspace2D
        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], Workspace2D))

        # Confirm the single period and its monitor are on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 26)

        # Confirm that the ADS workspace and the loaded workspace is the same, take a sample
        workspace_on_ads = AnalysisDataService.retrieve("5512p7_sans_nxs")
        workspace_monitor_on_ads = AnalysisDataService.retrieve("5512p7_sans_nxs_monitors")
        try:
            compare_workspaces(workspace_on_ads, workspace[SANSDataType.SampleScatter][6])
            compare_workspaces(workspace_monitor_on_ads, workspace_monitors[SANSDataType.SampleScatter][6])
            match = True
        except RuntimeError:
            match = False
        self.assertTrue(match)

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_can_load_isis_nexus_file_with_event_data_and_single_period(self):
        # Arrange
        load_factory = SANSLoadDataFactory()

        data_info = SANSStateDataISIS()
        ws_name_sample = "SANS2D00028827"
        data_info.sample_scatter = ws_name_sample
        ws_name_sample_transmission = "SANS2D00028784"
        data_info.sample_transmission = ws_name_sample_transmission
        ws_name_sample_direct = "SANS2D00028804"
        data_info.sample_direct = ws_name_sample_direct
        state = SANSStateISIS()
        state.data = data_info
        loader = load_factory.create_loader(state)

        # Act
        workspace, workspace_monitors = loader.execute(data_info, use_loaded=False, publish_to_ads=False)

        # Assert
        self.assertTrue(len(workspace) == 3)
        self.assertTrue(len(workspace_monitors) == 1)

        # Check the monitor workspaces -- we should only have the one of the sample scatter
        self.assertTrue(SANSDataType.SampleScatter in workspace_monitors.keys())
        self.assertTrue(isinstance(workspace_monitors[SANSDataType.SampleScatter][0], Workspace2D))

        # Check the actual workspaces
        self.assertTrue(SANSDataType.SampleScatter in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleScatter][0], EventWorkspace))
        self.assertTrue(SANSDataType.SampleTransmission in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleTransmission][0], Workspace2D))
        self.assertTrue(SANSDataType.SampleDirect in workspace.keys())
        self.assertTrue(isinstance(workspace[SANSDataType.SampleDirect][0], Workspace2D))

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 0)

    def test_that_calibration_workspace_is_applied_to_scatter_workspaces(self):
        pass

    def test_that_calibration_workspace_is_applied_to_scatter_workspaces(self):
        pass

    def test_that_calibration_workspace_is_applied_to_scatter_workspaces_and_can_make_use_of_ads(self):
        pass

if __name__ == '__main__':
    unittest.main()
