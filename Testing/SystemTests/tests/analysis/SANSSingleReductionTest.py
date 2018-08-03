# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import unittest
import stresstesting

import mantid  # noqa
from mantid.api import AlgorithmManager
from sans.user_file.state_director import StateDirectorISIS
from sans.state.data import get_data_builder
from sans.common.enums import (SANSFacility, ISISReductionMode, ReductionDimensionality, FitModeForMerge)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.file_information import SANSFileInformationFactory


# -----------------------------------------------
# Tests for the SANSSingleReduction algorithm
# -----------------------------------------------
class SANSSingleReductionTest(unittest.TestCase):
    def _load_workspace(self, state):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = state.property_manager
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", False)
        load_alg.setProperty("UseCached", False)
        load_alg.setProperty("MoveWorkspace", False)

        load_alg.setProperty("SampleScatterWorkspace", EMPTY_NAME)
        load_alg.setProperty("SampleScatterMonitorWorkspace", EMPTY_NAME)
        load_alg.setProperty("SampleTransmissionWorkspace", EMPTY_NAME)
        load_alg.setProperty("SampleDirectWorkspace", EMPTY_NAME)

        load_alg.setProperty("CanScatterWorkspace", EMPTY_NAME)
        load_alg.setProperty("CanScatterMonitorWorkspace", EMPTY_NAME)
        load_alg.setProperty("CanTransmissionWorkspace", EMPTY_NAME)
        load_alg.setProperty("CanDirectWorkspace", EMPTY_NAME)

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        sample_scatter = load_alg.getProperty("SampleScatterWorkspace").value
        sample_scatter_monitor_workspace = load_alg.getProperty("SampleScatterMonitorWorkspace").value
        transmission_workspace = load_alg.getProperty("SampleTransmissionWorkspace").value
        direct_workspace = load_alg.getProperty("SampleDirectWorkspace").value

        can_scatter_workspace = load_alg.getProperty("CanScatterWorkspace").value
        can_scatter_monitor_workspace = load_alg.getProperty("CanScatterMonitorWorkspace").value
        can_transmission_workspace = load_alg.getProperty("CanTransmissionWorkspace").value
        can_direct_workspace = load_alg.getProperty("CanDirectWorkspace").value

        return sample_scatter, sample_scatter_monitor_workspace, transmission_workspace, direct_workspace, \
               can_scatter_workspace, can_scatter_monitor_workspace, can_transmission_workspace, can_direct_workspace  # noqa

    def _run_single_reduction(self, state, sample_scatter, sample_monitor, sample_transmission=None, sample_direct=None,
                              can_scatter=None, can_monitor=None, can_transmission=None, can_direct=None,
                              output_settings=None):
        single_reduction_name = "SANSSingleReduction"
        state_dict = state.property_manager

        single_reduction_options = {"SANSState": state_dict,
                                    "SampleScatterWorkspace": sample_scatter,
                                    "SampleScatterMonitorWorkspace": sample_monitor,
                                    "UseOptimizations": False}
        if sample_transmission:
            single_reduction_options.update({"SampleTransmissionWorkspace": sample_transmission})

        if sample_direct:
            single_reduction_options.update({"SampleDirectWorkspace": sample_direct})

        if can_scatter:
            single_reduction_options.update({"CanScatterWorkspace": can_scatter})

        if can_monitor:
            single_reduction_options.update({"CanScatterMonitorWorkspace": can_monitor})

        if can_transmission:
            single_reduction_options.update({"CanTransmissionWorkspace": can_transmission})

        if can_direct:
            single_reduction_options.update({"CanDirectWorkspace": can_direct})

        if output_settings:
            single_reduction_options.update(output_settings)

        single_reduction_alg = create_unmanaged_algorithm(single_reduction_name, **single_reduction_options)

        # Act
        single_reduction_alg.execute()
        self.assertTrue(single_reduction_alg.isExecuted())
        return single_reduction_alg

    def _compare_workspace(self, workspace, reference_file_name, check_spectra_map=True):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name,
                        "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reference_workspace = load_alg.getProperty("OutputWorkspace").value

        # Compare reference file with the output_workspace
        # We need to disable the instrument comparison, it takes way too long
        # We need to disable the sample -- Not clear why yet
        # operation how many entries can be found in the sample logs
        compare_name = "CompareWorkspaces"
        compare_options = {"Workspace1": workspace,
                           "Workspace2": reference_workspace,
                           "Tolerance": 1e-6,
                           "CheckInstrument": False,
                           "CheckSample": False,
                           "ToleranceRelErr": True,
                           "CheckAllData": True,
                           "CheckMasking": True,
                           "CheckType": True,
                           "CheckAxes": True,
                           "CheckSpectraMap": check_spectra_map}
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value
        self.assertTrue(result)

    def test_that_single_reduction_evaluates_LAB(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_can_scatter("SANS2D00034481")
        data_builder.set_can_transmission("SANS2D00034502")
        data_builder.set_can_direct("SANS2D00034461")

        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_info, file_information)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        # Set the reduction mode to LAB
        user_file_director.set_reduction_builder_reduction_mode(ISISReductionMode.LAB)

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN -- Remove when appropriate
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        user_file_director.set_compatibility_builder_use_compatibility_mode(True)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        state = user_file_director.construct()
        state.adjustment.show_transmission = True

        # Load the sample workspaces
        sample, sample_monitor, transmission_workspace, direct_workspace, can, can_monitor, \
        can_transmission, can_direct = self._load_workspace(state)  # noqa

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(state, sample_scatter=sample,
                                                          sample_transmission=transmission_workspace,
                                                          sample_direct=direct_workspace,
                                                          sample_monitor=sample_monitor,
                                                          can_scatter=can,
                                                          can_monitor=can_monitor,
                                                          can_transmission=can_transmission,
                                                          can_direct=can_direct,
                                                          output_settings=output_settings)
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceLAB").value
        calculated_transmission = single_reduction_alg.getProperty("OutputWorkspaceCalculatedTransmission").value
        unfitted_transmission = single_reduction_alg.getProperty("OutputWorkspaceUnfittedTransmission").value
        calculated_transmission_can = single_reduction_alg.getProperty("OutputWorkspaceCalculatedTransmissionCan").value
        unfitted_transmission_can = single_reduction_alg.getProperty("OutputWorkspaceUnfittedTransmissionCan").value

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_LAB_1D.nxs"
        self._compare_workspace(output_workspace, reference_file_name)

        calculated_transmission_reference_file = "SANS2D_ws_D20_calculated_transmission_reference_LAB.nxs"
        unfitted_transmission_reference_file = "SANS2D_ws_D20_unfitted_transmission_reference_LAB.nxs"
        calculated_transmission_reference_file_can = "SANS2D_ws_D20_calculated_transmission_reference_LAB_can.nxs"
        unfitted_transmission_reference_file_can = "SANS2D_ws_D20_unfitted_transmission_reference_LAB_can.nxs"
        self._compare_workspace(calculated_transmission, calculated_transmission_reference_file, check_spectra_map=False)
        self._compare_workspace(unfitted_transmission, unfitted_transmission_reference_file)
        self._compare_workspace(calculated_transmission_can, calculated_transmission_reference_file_can,
                                check_spectra_map=False)
        self._compare_workspace(unfitted_transmission_can, unfitted_transmission_reference_file_can)

    def test_that_single_reduction_evaluates_HAB(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_can_scatter("SANS2D00034481")
        data_builder.set_can_transmission("SANS2D00034502")
        data_builder.set_can_direct("SANS2D00034461")

        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_info, file_information)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        # Set the reduction mode to LAB
        user_file_director.set_reduction_builder_reduction_mode(ISISReductionMode.HAB)

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN -- Remove when appropriate
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        user_file_director.set_compatibility_builder_use_compatibility_mode(True)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        state = user_file_director.construct()

        # Load the sample workspaces
        sample, sample_monitor, transmission_workspace, direct_workspace, can, can_monitor,\
        can_transmission, can_direct = self._load_workspace(state)  # noqa

        # Act
        output_settings = {"OutputWorkspaceHAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(state, sample_scatter=sample,
                                                          sample_transmission=transmission_workspace,
                                                          sample_direct=direct_workspace,
                                                          sample_monitor=sample_monitor,
                                                          can_scatter=can,
                                                          can_monitor=can_monitor,
                                                          can_transmission=can_transmission,
                                                          can_direct=can_direct,
                                                          output_settings=output_settings)
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceHAB").value

        # # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_HAB_1D.nxs"
        self._compare_workspace(output_workspace, reference_file_name)

    def test_that_single_reduction_evaluates_merged(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_can_scatter("SANS2D00034481")
        data_builder.set_can_transmission("SANS2D00034502")
        data_builder.set_can_direct("SANS2D00034461")

        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_info, file_information)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        # Set the reduction mode to LAB
        user_file_director.set_reduction_builder_reduction_mode(ISISReductionMode.Merged)
        user_file_director.set_reduction_builder_merge_fit_mode(FitModeForMerge.Both)
        user_file_director.set_reduction_builder_merge_scale(1.0)
        user_file_director.set_reduction_builder_merge_shift(0.0)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN -- Remove when appropriate
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        user_file_director.set_compatibility_builder_use_compatibility_mode(True)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        state = user_file_director.construct()

        # Load the sample workspaces
        sample, sample_monitor, transmission_workspace, direct_workspace, \
        can, can_monitor, can_transmission, can_direct = self._load_workspace(state)  # noqa

        # Act
        output_settings = {"OutputWorkspaceMerged": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(state, sample_scatter=sample,
                                                          sample_transmission=transmission_workspace,
                                                          sample_direct=direct_workspace,
                                                          sample_monitor=sample_monitor,
                                                          can_scatter=can,
                                                          can_monitor=can_monitor,
                                                          can_transmission=can_transmission,
                                                          can_direct=can_direct,
                                                          output_settings=output_settings)
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceMerged").value
        output_scale_factor = single_reduction_alg.getProperty("OutScaleFactor").value
        output_shift_factor = single_reduction_alg.getProperty("OutShiftFactor").value

        tolerance = 1e-6
        expected_shift = 0.00278452
        expected_scale = 0.81439154

        self.assertTrue(abs(expected_shift - output_shift_factor) < tolerance)
        self.assertTrue(abs(expected_scale - output_scale_factor) < tolerance)

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_Merged_1D.nxs"
        self._compare_workspace(output_workspace, reference_file_name)

    def test_that_single_reduction_evaluates_LAB_for_2D_reduction(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_can_scatter("SANS2D00034481")
        data_builder.set_can_transmission("SANS2D00034502")
        data_builder.set_can_direct("SANS2D00034461")

        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_info, file_information)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        # Set the reduction mode to LAB
        user_file_director.set_reduction_builder_reduction_mode(ISISReductionMode.LAB)
        user_file_director.set_reduction_builder_reduction_dimensionality(ReductionDimensionality.TwoDim)
        user_file_director.set_convert_to_q_builder_reduction_dimensionality(ReductionDimensionality.TwoDim)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN -- Remove when appropriate
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        user_file_director.set_compatibility_builder_use_compatibility_mode(True)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        state = user_file_director.construct()

        # Load the sample workspaces
        sample, sample_monitor, transmission_workspace, direct_workspace, can, can_monitor, \
        can_transmission, can_direct = self._load_workspace(state)  # noqa

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(state, sample_scatter=sample,
                                                          sample_transmission=transmission_workspace,
                                                          sample_direct=direct_workspace,
                                                          sample_monitor=sample_monitor,
                                                          can_scatter=can,
                                                          can_monitor=can_monitor,
                                                          can_transmission=can_transmission,
                                                          can_direct=can_direct,
                                                          output_settings=output_settings)
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceLAB").value

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_LAB_2D.nxs"
        self._compare_workspace(output_workspace, reference_file_name)


class SANSReductionRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSSingleReductionTest, 'test'))
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
