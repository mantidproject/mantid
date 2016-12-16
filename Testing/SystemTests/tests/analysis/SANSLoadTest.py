# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import stresstesting

from mantid.dataobjects import (Workspace2D, EventWorkspace)
from mantid.api import (AnalysisDataService, AlgorithmManager)

from SANS.Load.SANSLoadData import SANSLoadDataFactory
from SANS2.Common.SANSLogTagger import has_tag
from SANS2.Common.SANSConstants import SANSConstants

# Not clear why the names in the module are not found by Pylint, but it seems to get confused. Hence this check
# needs to be disabled here.
# pylint: disable=no-name-in-module
from SANS2.State.SANSStateData import SANSStateDataISIS
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


# -----------------------------------------------
# Tests for the Load factory
# -----------------------------------------------
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


# -----------------------------------------------
# Tests for the SANSLoad algorithm
# -----------------------------------------------
class SANSLoadTest(unittest.TestCase):
    @staticmethod
    def _get_simple_state(sample_scatter, sample_trans=None, sample_direct=None,
                          can_scatter=None, can_trans=None, can_direct=None, calibration=None,
                          sample_scatter_period=None, sample_trans_period=None, sample_direct_period=None,
                          can_scatter_period=None, can_trans_period=None, can_direct_period=None):
        data_info = SANSStateDataISIS()
        data_info.sample_scatter = sample_scatter

        # Set the file names
        if sample_trans is not None:
            data_info.sample_transmission = sample_trans

        if sample_direct is not None:
            data_info.sample_direct = sample_direct

        if can_scatter is not None:
            data_info.can_scatter = can_scatter

        if can_trans is not None:
            data_info.can_transmission = can_trans

        if can_direct is not None:
            data_info.can_direct = can_direct

        # Set the periods
        if sample_scatter_period is not None:
            data_info.sample_scatter_period = sample_scatter_period

        if sample_trans_period is not None:
            data_info.sample_transmission_period = sample_trans_period

        if sample_direct_period is not None:
            data_info.sample_direct_period = sample_direct_period

        if can_scatter_period is not None:
            data_info.can_scatter_period = can_scatter_period

        if can_trans_period is not None:
            data_info.can_transmission_period = can_trans_period

        if can_direct_period is not None:
            data_info.can_direct_period = can_direct_period

        # Add the calibration
        if calibration is not None:
            data_info.calibration = calibration

        state = SANSStateISIS()
        state.data = data_info
        return state

    def _evaluate_workspace_type(self, load_alg, num_workspaces, workspace_name, workspace_type, index):
        if num_workspaces == 1:
            ws = load_alg.getProperty(workspace_name).value
            self.assertTrue(isinstance(ws, workspace_type[index]))
        elif num_workspaces > 1:
            ws = load_alg.getProperty(workspace_name).value
            self.assertTrue(isinstance(ws, workspace_type[index]))
            for ind in range(1, num_workspaces + 1):
                output_name = workspace_name + "_" + str(ind)
                ws = load_alg.getProperty(output_name).value
                self.assertTrue(isinstance(ws, workspace_type[index]))
        else:
            ws = load_alg.getProperty(workspace_name).value
            self.assertTrue(ws is None)

    def _do_test_output(self, load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type):
        #  Check the number of workspaces
        tags_numbers = ["NumberOfSampleScatterWorkspaces", "NumberOfSampleTransmissionWorkspaces",
                        "NumberOfSampleDirectWorkspaces", "NumberOfCanScatterWorkspaces",
                        "NumberOfCanTransmissionWorkspaces", "NumberOfCanDirectWorkspaces"]
        for num_workspaces, num_name in zip(expected_number_of_workspaces, tags_numbers):
            number_of_workspaces = load_alg.getProperty(num_name).value
            self.assertTrue(number_of_workspaces == num_workspaces)

        # Check that workspaces were loaded
        tags_workspaces = ["SampleScatterWorkspace", "SampleTransmissionWorkspace",
                           "SampleDirectWorkspace", "CanScatterWorkspace",
                           "CanTransmissionWorkspace", "CanDirectWorkspace"]
        index = 0
        for num_workspaces, workspace_name in zip(expected_number_of_workspaces, tags_workspaces):
            self._evaluate_workspace_type(load_alg, num_workspaces, workspace_name, workspace_type, index)
            index += 1

        # Check for the monitor workspaces
        num_monitor_workspaces = [expected_number_of_workspaces[0], expected_number_of_workspaces[3]]
        tags_monitors = ["SampleScatterMonitorWorkspace", "CanScatterMonitorWorkspace"]
        workspace_type_monitor = [Workspace2D, Workspace2D]
        index = 0
        for num_workspaces, workspace_name in zip(num_monitor_workspaces, tags_monitors):
            self._evaluate_workspace_type(load_alg, num_workspaces, workspace_name, workspace_type_monitor, index)
            index += 1

        # Confirm there is nothing on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == expected_number_on_ads)

    @staticmethod
    def _has_calibration_been_applied(load_alg):
        sample_workspace = load_alg.getProperty("SampleScatterWorkspace").value
        return has_tag(SANSConstants.Calibration.calibration_workspace_tag, sample_workspace)

    def _run_load(self, state, publish_to_cache, use_cached, move_workspace):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = state.property_manager
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", publish_to_cache)
        load_alg.setProperty("UseCached", use_cached)
        load_alg.setProperty("MoveWorkspace", move_workspace)

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        return load_alg

    def test_that_runs_for_isis_nexus_file_with_event_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00028827",
                                               sample_trans="SANS2D00028784",
                                               sample_direct="SANS2D00028804",
                                               calibration="TUBE_SANS2D_BOTH_27345_20Mar15.nxs")

        # Act
        load_alg = self._run_load(state, publish_to_cache=False, use_cached=False, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [1, 1, 1, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [EventWorkspace, Workspace2D, Workspace2D, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertTrue(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_isis_nexus_file_with_histogram_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00000808",
                                               sample_trans="SANS2D00028784",
                                               sample_direct="SANS2D00028804")

        # Act
        load_alg = self._run_load(state, publish_to_cache=False, use_cached=False, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [1, 1, 1, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, Workspace2D, Workspace2D, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_raw_file_with_histogram_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00000808.raw",
                                               sample_trans="SANS2D00028784",
                                               sample_direct="SANS2D00028804")

        # Act
        load_alg = self._run_load(state, publish_to_cache=False, use_cached=False, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [1, 1, 1, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, Workspace2D, Workspace2D, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_isis_nexus_file_with_histogram_data_and_multi_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00005512.nxs")

        # Act
        load_alg = self._run_load(state, publish_to_cache=False, use_cached=False, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [13, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_isis_nexus_file_with_histogram_data_and_multi_period_and_select_single_period(self):
        # Arrange
        special_selection_on_group = 3
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00005512.nxs",
                                               sample_scatter_period=special_selection_on_group)

        # Act
        load_alg = self._run_load(state, publish_to_cache=False, use_cached=False, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [1, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_can_load_isis_nexus_file_with_histogram_data_and_multi_period_and_add_to_ads(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00005512.nxs")

        # Act
        load_alg = self._run_load(state, publish_to_cache=True, use_cached=True, move_workspace=False)

        # Assert
        expected_number_of_workspaces = [13, 0, 0, 0, 0, 0]
        expected_number_on_ads = 2*13
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Confirm the single period and its monitor are on the ADS
        workspaces_on_the_ads = AnalysisDataService.getObjectNames()
        self.assertTrue(len(workspaces_on_the_ads) == 26)

        # Confirm that the ADS workspace and the loaded workspace is the same, take a sample
        try:
            workspace_on_ads = AnalysisDataService.retrieve("5512p7_sans_nxs")
            workspace_monitor_on_ads = AnalysisDataService.retrieve("5512p7_sans_nxs_monitors")

            workspace = load_alg.getProperty("SampleScatterWorkspace_7").value
            workspace_monitor = load_alg.getProperty("SampleScatterMonitorWorkspace_7").value

            compare_workspaces(workspace_on_ads, workspace)
            compare_workspaces(workspace_monitor_on_ads, workspace_monitor)
            match = True
        except RuntimeError:
            match = False
        self.assertTrue(match)

        # Cleanup
        remove_all_workspaces_from_ads()


class SANSLoadDataRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSLoadFactoryTest, 'test'))
        # suite.addTest(unittest.makeSuite(SANSLoadDataTest, 'test'))
        suite.addTest(unittest.makeSuite(SANSLoadTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success


if __name__ == '__main__':
    unittest.main()
