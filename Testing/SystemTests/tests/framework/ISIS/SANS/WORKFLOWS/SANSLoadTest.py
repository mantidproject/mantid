# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid.dataobjects import Workspace2D, EventWorkspace
from mantid.api import AnalysisDataService, AlgorithmManager

from sans.algorithm_detail.load_data import SANSLoadDataFactory
from sans.common.log_tagger import has_tag
from sans.common.constants import CALIBRATION_WORKSPACE_TAG, SANS_FILE_TAG

# Not clear why the names in the module are not found by Pylint, but it seems to get confused. Hence this check
# needs to be disabled here.
# pylint: disable=no-name-in-module
from sans.state.Serializer import Serializer
from sans.test_helper.test_director import TestDirector
from sans.common.enums import SANSFacility, SANSInstrument
from sans.state.StateObjects.StateData import get_data_builder
from sans.common.file_information import SANSFileInformationFactory


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
        file_information_factory = SANSFileInformationFactory()

        ws_name_sample = "SANS2D00022024"
        file_information = file_information_factory.create_sans_file_information(ws_name_sample)
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter(ws_name_sample)
        data = data_builder.build()

        # Get the sample state
        test_director = TestDirector()
        test_director.set_states(data_state=data)
        state = test_director.construct()

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
@ISISSansSystemTest(SANSInstrument.LARMOR, SANSInstrument.SANS2D)
class SANSLoadTest(unittest.TestCase):
    @staticmethod
    def _get_simple_state(
        sample_scatter,
        sample_trans=None,
        sample_direct=None,
        can_scatter=None,
        can_trans=None,
        can_direct=None,
        calibration=None,
        sample_scatter_period=None,
        sample_trans_period=None,
        sample_direct_period=None,
        can_scatter_period=None,
        can_trans_period=None,
        can_direct_period=None,
    ):
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(sample_scatter)

        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter(sample_scatter)

        # Set the file names
        if sample_trans is not None:
            data_builder.set_sample_transmission(sample_trans)

        if sample_direct is not None:
            data_builder.set_sample_direct(sample_direct)

        if can_scatter is not None:
            data_builder.set_can_scatter(can_scatter)

        if can_trans is not None:
            data_builder.set_can_transmission(can_trans)

        if can_direct is not None:
            data_builder.set_can_direct(can_direct)

        # Set the periods
        if sample_scatter_period is not None:
            data_builder.set_sample_scatter_period(sample_scatter_period)

        if sample_trans_period is not None:
            data_builder.set_sample_transmission_period(sample_trans_period)

        if sample_direct_period is not None:
            data_builder.set_sample_direct_period(sample_direct_period)

        if can_scatter_period is not None:
            data_builder.set_can_scatter_period(can_scatter_period)

        if can_trans_period is not None:
            data_builder.set_can_transmission_period(can_trans_period)

        if can_direct_period is not None:
            data_builder.set_can_direct_period(can_direct_period)

        data_info = data_builder.build()

        # Get the sample state
        test_director = TestDirector()
        test_director.set_states(data_state=data_info)

        state = test_director.construct()
        state.adjustment.calibration = calibration

        return state

    def _evaluate_workspace_type(self, load_alg, num_workspaces, workspace_name, workspace_type, index):
        if num_workspaces == 1:
            ws = load_alg.getProperty(workspace_name).value
            self.assertTrue(isinstance(ws, workspace_type[index]))
        elif num_workspaces > 1:
            for ind in range(1, num_workspaces + 1):
                output_name = workspace_name + "_" + str(ind)
                ws = load_alg.getProperty(output_name).value
                self.assertTrue(isinstance(ws, workspace_type[index]))
        else:
            ws = load_alg.getProperty(workspace_name).value
            self.assertEqual(ws, None)

    def _do_test_output(self, load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type):
        #  Check the number of workspaces
        tags_numbers = [
            "NumberOfSampleScatterWorkspaces",
            "NumberOfSampleTransmissionWorkspaces",
            "NumberOfSampleDirectWorkspaces",
            "NumberOfCanScatterWorkspaces",
            "NumberOfCanTransmissionWorkspaces",
            "NumberOfCanDirectWorkspaces",
        ]
        for num_workspaces, num_name in zip(expected_number_of_workspaces, tags_numbers):
            number_of_workspaces = load_alg.getProperty(num_name).value
            self.assertEqual(number_of_workspaces, num_workspaces)

        # Check that workspaces were loaded
        tags_workspaces = [
            "SampleScatterWorkspace",
            "SampleTransmissionWorkspace",
            "SampleDirectWorkspace",
            "CanScatterWorkspace",
            "CanTransmissionWorkspace",
            "CanDirectWorkspace",
        ]
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
        self.assertEqual(len(workspaces_on_the_ads), expected_number_on_ads)

    @staticmethod
    def _has_calibration_been_applied(load_alg):
        sample_workspace = load_alg.getProperty("SampleScatterWorkspace").value
        if sample_workspace is None:
            sample_workspace = load_alg.getProperty("SampleScatterWorkspace_1").value
        has_calibration_tag = has_tag(CALIBRATION_WORKSPACE_TAG, sample_workspace)
        has_file_tag = has_tag(SANS_FILE_TAG, sample_workspace)
        return has_calibration_tag and has_file_tag

    @staticmethod
    def _run_load(
        state, publish_to_cache, use_cached, move_workspace=False, beam_coordinates=None, component=None, output_workspace_names=None
    ):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.setRethrows(True)
        load_alg.initialize()

        state_dict = Serializer.to_json(state)
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", publish_to_cache)
        load_alg.setProperty("UseCached", use_cached)
        if move_workspace:
            load_alg.setProperty("Component", component)
            load_alg.setProperty("BeamCoordinates", beam_coordinates)

        if output_workspace_names:
            for name, value in output_workspace_names.items():
                load_alg.setProperty(name, value)

        # Act
        load_alg.execute()
        # self.assertTrue(load_alg.isExecuted())
        return load_alg

    def test_that_when_transmission_is_event_monitor_is_used(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(
            sample_scatter="SANS2D00028827", sample_trans="SANS2D00028827", sample_direct="SANS2D00028827"
        )

        # Act
        output_workspace_names = {
            "SampleScatterWorkspace": "sample_scatter",
            "SampleScatterMonitorWorkspace": "sample_monitor_scatter",
            "SampleTransmissionWorkspace": "sample_transmission",
            "SampleDirectWorkspace": "sample_direct",
        }

        kwargs = {
            "state": state,
            "publish_to_cache": True,
            "use_cached": True,
            "move_workspace": False,
            "output_workspace_names": output_workspace_names,
        }
        load_alg = self._run_load(**kwargs)
        transmission_workspace = load_alg.getProperty("SampleTransmissionWorkspace").value
        self.assertEqual(transmission_workspace.getNumberHistograms(), 8)

    def test_that_runs_for_isis_nexus_file_with_event_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(
            sample_scatter="SANS2D00028827",
            sample_trans="SANS2D00028784",
            sample_direct="SANS2D00028804",
            calibration="TUBE_SANS2D_BOTH_27345_20Mar15.nxs",
        )

        # Act
        output_workspace_names = {
            "SampleScatterWorkspace": "sample_scatter",
            "SampleScatterMonitorWorkspace": "sample_monitor_scatter",
            "SampleTransmissionWorkspace": "sample_transmission",
            "SampleDirectWorkspace": "sample_direct",
        }

        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 1, 1, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [EventWorkspace, Workspace2D, Workspace2D, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertTrue(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_isis_nexus_file_with_histogram_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(
            sample_scatter="SANS2D00000808", sample_trans="SANS2D00028784", sample_direct="SANS2D00028804"
        )

        # Act
        output_workspace_names = {
            "SampleScatterWorkspace": "sample_scatter",
            "SampleScatterMonitorWorkspace": "sample_monitor_scatter",
            "SampleTransmissionWorkspace": "sample_transmission",
            "SampleDirectWorkspace": "sample_direct",
        }
        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 1, 1, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, Workspace2D, Workspace2D, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_runs_for_raw_file_with_histogram_data_and_single_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(
            sample_scatter="SANS2D00000808.raw", sample_trans="SANS2D00028784", sample_direct="SANS2D00028804"
        )

        # Act
        output_workspace_names = {
            "SampleScatterWorkspace": "sample_scatter",
            "SampleScatterMonitorWorkspace": "sample_monitor_scatter",
            "SampleTransmissionWorkspace": "sample_transmission",
            "SampleDirectWorkspace": "sample_direct",
        }
        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

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
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

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
        state = SANSLoadTest._get_simple_state(sample_scatter="SANS2D00005512.nxs", sample_scatter_period=special_selection_on_group)

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

    def test_that_can_load_isis_nexus_file_with_event_data_and_multi_period(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(
            sample_scatter="LARMOR00013065.nxs", calibration="80tubeCalibration_18-04-2016_r9330-9335.nxs"
        )

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=True, use_cached=True, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [4, 0, 0, 0, 0, 0]
        expected_number_on_ads = 1
        workspace_type = [EventWorkspace, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertTrue(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Confirm that the ADS workspace contains the calibration file
        try:
            AnalysisDataService.retrieve("80tubeCalibration_18-04-2016_r9330-9335")
            on_ads = True
        except RuntimeError:
            on_ads = False
        self.assertTrue(on_ads)

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_runs_for_isis_nexus_file_with_event_data_and_multi_period_and_select_single_period(self):
        # Arrange
        special_selection_on_group = 3
        state = SANSLoadTest._get_simple_state(sample_scatter="LARMOR00013065.nxs", sample_scatter_period=special_selection_on_group)

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=True, use_cached=True, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [EventWorkspace, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration has not been added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_can_load_single_period_from_added_multi_period_histogram_file(self):
        # Arrange
        special_selection_on_group = 7
        state = SANSLoadTest._get_simple_state(
            sample_scatter="AddedMultiPeriodHistogram-add.nxs", sample_scatter_period=special_selection_on_group
        )

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=True, use_cached=True, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_can_load_all_periods_from_added_multi_period_histogram_file(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="AddedMultiPeriodHistogram-add.nxs")

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=False, use_cached=False, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [13, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [Workspace2D, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_can_load_single_period_from_added_multi_period_event_file(self):
        # Arrange
        special_selection_on_group = 2
        state = SANSLoadTest._get_simple_state(
            sample_scatter="AddedMultiPeriodEvent-add.nxs", sample_scatter_period=special_selection_on_group
        )

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=True, use_cached=True, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [1, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [EventWorkspace, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Cleanup
        remove_all_workspaces_from_ads()

    def test_that_can_load_all_periods_from_added_multi_period_event_file(self):
        # Arrange
        state = SANSLoadTest._get_simple_state(sample_scatter="AddedMultiPeriodEvent-add.nxs")

        # Act
        output_workspace_names = {"SampleScatterWorkspace": "sample_scatter", "SampleScatterMonitorWorkspace": "sample_monitor_scatter"}
        load_alg = self._run_load(
            state, publish_to_cache=True, use_cached=True, move_workspace=False, output_workspace_names=output_workspace_names
        )

        # Assert
        expected_number_of_workspaces = [4, 0, 0, 0, 0, 0]
        expected_number_on_ads = 0
        workspace_type = [EventWorkspace, None, None, None, None, None]
        self._do_test_output(load_alg, expected_number_of_workspaces, expected_number_on_ads, workspace_type)

        # Check that calibration is added
        self.assertFalse(SANSLoadTest._has_calibration_been_applied(load_alg))

        # Cleanup
        remove_all_workspaces_from_ads()


class SANSLoadDataRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSLoadFactoryTest, "test"))
        suite.addTest(unittest.makeSuite(SANSLoadTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
