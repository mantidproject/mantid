# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
from os import path

import systemtesting
from systemtesting import MantidSystemTest
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid import config
from mantid.api import AlgorithmManager, WorkspaceGroup
from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.enums import SANSFacility, ReductionMode, ReductionDimensionality, FitModeForMerge, SANSInstrument
from SANS.sans.common.file_information import SANSFileInformationFactory
from SANS.sans.common.general_functions import create_unmanaged_algorithm
from sans.state.Serializer import Serializer
from sans.state.StateObjects.StateData import get_data_builder

# ----------------------------------------------------------------------------------------------------------------------
# Base class containing useful functions for the tests
# ----------------------------------------------------------------------------------------------------------------------
from sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SingleReductionTest(unittest.TestCase):
    def _load_workspace(self, state):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = Serializer.to_json(state)
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", False)
        load_alg.setProperty("UseCached", False)

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

        return (
            sample_scatter,
            sample_scatter_monitor_workspace,
            transmission_workspace,
            direct_workspace,
            can_scatter_workspace,
            can_scatter_monitor_workspace,
            can_transmission_workspace,
            can_direct_workspace,
        )

    def _compare_to_reference(self, workspace, reference_file_name, check_spectra_map=True, mismatch_name=""):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.setProperty("OutputWorkspace", reference_file_name.split(".")[0])
        load_alg.execute()
        reference_workspace = load_alg.getProperty("OutputWorkspace").value

        # Compare reference file with the output_workspace
        self._compare_workspace(workspace, reference_workspace, check_spectra_map=check_spectra_map, mismatch_name=mismatch_name)

    def _compare_workspace(self, input_workspace, reference_workspace, check_spectra_map=True, tolerance=1e-6, mismatch_name=""):
        # We need to disable the instrument comparison, it takes way too long
        # We need to disable the sample -- Not clear why yet
        # operation how many entries can be found in the sample logs
        compare_name = "CompareWorkspaces"
        compare_options = {
            "Workspace1": input_workspace.getItem(0),
            "Workspace2": reference_workspace,
            "Tolerance": tolerance,
            "CheckInstrument": False,
            "CheckSample": False,
            "ToleranceRelErr": True,
            "CheckAllData": True,
            "CheckMasking": True,
            "CheckType": True,
            "CheckAxes": True,
            "CheckSpectraMap": check_spectra_map,
        }
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value

        if not result:
            self._save_output(input_workspace, mismatch_name)

        self.assertTrue(result)

    def _save_output(self, workspace, mismatch_name):
        # Save the workspace out
        f_name = path.join(config.getString("defaultsave.directory"), mismatch_name)
        save_name = "SaveNexus"
        save_options = {"Filename": f_name, "InputWorkspace": workspace}
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.execute()

    def _run_single_reduction(
        self,
        state,
        sample_scatter,
        sample_monitor,
        sample_transmission=None,
        sample_direct=None,
        can_scatter=None,
        can_monitor=None,
        can_transmission=None,
        can_direct=None,
        output_settings=None,
        event_slice_optimisation=False,
        save_can=False,
        use_optimizations=False,
    ):
        single_reduction_name = "SANSSingleReduction"
        ver = 1 if not event_slice_optimisation else 2
        state_dict = Serializer.to_json(state)

        single_reduction_options = {
            "SANSState": state_dict,
            "SampleScatterWorkspace": sample_scatter,
            "SampleScatterMonitorWorkspace": sample_monitor,
            "UseOptimizations": use_optimizations,
            "SaveCan": save_can,
        }
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

        single_reduction_alg = create_unmanaged_algorithm(single_reduction_name, version=ver, **single_reduction_options)

        # Act
        single_reduction_alg.execute()
        self.assertTrue(single_reduction_alg.isExecuted())
        return single_reduction_alg


# ----------------------------------------------------------------------------------------------------------------------
# Test version 1 of SANSSingleReduction
# ----------------------------------------------------------------------------------------------------------------------
class SANSSingleReductionTest(SingleReductionTest):
    def test_single_reduction_with_save_can(self):
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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        # Set the reduction mode to LAB
        state.reduction.reduction_mode = ReductionMode.LAB
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.data = data_info

        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        state.compatibility.use_compatibility_mode = True

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            save_can=True,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
        )
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceLAB").value

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_LAB_1D.nxs"
        self._compare_to_reference(
            output_workspace, reference_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(reference_file_name)
        )

        self.assertIsInstance(single_reduction_alg.getProperty("OutputWorkspaceLABCan").value, WorkspaceGroup)
        self.assertIsNone(single_reduction_alg.getProperty("OutputWorkspaceHABCan").value)

    def test_that_single_reduction_evaluates_LAB(self):
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_can_scatter("SANS2D00034481")
        data_builder.set_can_transmission("SANS2D00034502")
        data_builder.set_can_direct("SANS2D00034461")

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        # Set the reduction mode to LAB
        state.reduction.reduction_mode = ReductionMode.LAB
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.data = data_info

        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        state.compatibility.use_compatibility_mode = True

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
        )
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceLAB").value
        calculated_transmission = single_reduction_alg.getProperty("OutputWorkspaceCalculatedTransmission").value
        unfitted_transmission = single_reduction_alg.getProperty("OutputWorkspaceUnfittedTransmission").value
        calculated_transmission_can = single_reduction_alg.getProperty("OutputWorkspaceCalculatedTransmissionCan").value
        unfitted_transmission_can = single_reduction_alg.getProperty("OutputWorkspaceUnfittedTransmissionCan").value

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_LAB_1D.nxs"
        self._compare_to_reference(
            output_workspace, reference_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(reference_file_name)
        )

        calculated_transmission_reference_file = "SANS2D_ws_D20_calculated_transmission_reference_LAB.nxs"
        unfitted_transmission_reference_file = "SANS2D_ws_D20_unfitted_transmission_reference_LAB.nxs"
        calculated_transmission_reference_file_can = "SANS2D_ws_D20_calculated_transmission_reference_LAB_can.nxs"
        unfitted_transmission_reference_file_can = "SANS2D_ws_D20_unfitted_transmission_reference_LAB_can.nxs"
        self._compare_to_reference(
            calculated_transmission,
            calculated_transmission_reference_file,
            check_spectra_map=False,
            mismatch_name=MantidSystemTest.mismatchWorkspaceName(calculated_transmission_reference_file),
        )
        self._compare_to_reference(
            unfitted_transmission,
            unfitted_transmission_reference_file,
            mismatch_name=MantidSystemTest.mismatchWorkspaceName(unfitted_transmission_reference_file),
        )
        self._compare_to_reference(
            calculated_transmission_can,
            calculated_transmission_reference_file_can,
            check_spectra_map=False,
            mismatch_name=MantidSystemTest.mismatchWorkspaceName(calculated_transmission_reference_file_can),
        )
        self._compare_to_reference(
            unfitted_transmission_can,
            unfitted_transmission_reference_file_can,
            mismatch_name=MantidSystemTest.mismatchWorkspaceName(unfitted_transmission_reference_file_can),
        )

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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.reduction.reduction_mode = ReductionMode.HAB
        state.compatibility.use_compatibility_mode = True
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.data = data_info

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceHAB": "TestSingleReductionEvalHab"}
        single_reduction_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
        )
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceHAB").value

        # # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_HAB_1D.nxs"
        self._compare_to_reference(
            output_workspace, reference_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(reference_file_name)
        )

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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.reduction.reduction_mode = ReductionMode.MERGED
        state.reduction.merge_fit_mode = FitModeForMerge.BOTH
        state.reduction.merge_scale = 1.0
        state.reduction.merge_shift = 0.0
        state.data = data_info

        state.compatibility.use_compatibility_mode = True

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceMerged": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
        )
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceMerged").value
        output_hab_scaled = single_reduction_alg.getProperty("OutputWorkspaceHABScaled").value
        output_scale_factor = single_reduction_alg.getProperty("OutScaleFactor").value
        output_shift_factor = single_reduction_alg.getProperty("OutShiftFactor").value

        tolerance = 1e-6
        expected_shift = 0.00278452
        expected_scale = 0.81439387

        self.assertTrue(abs(expected_shift - output_shift_factor) < tolerance)
        self.assertTrue(abs(expected_scale - output_scale_factor) < tolerance)

        scaled_file_name = "SANSSingleReduction_Scaled_HAB.nxs"
        self._compare_to_reference(
            output_hab_scaled, scaled_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(scaled_file_name)
        )

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_Merged_1D.nxs"
        self._compare_to_reference(
            output_workspace, reference_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(reference_file_name)
        )

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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)

        # Set the reduction mode to LAB
        state.reduction.reduction_mode = ReductionMode.LAB
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.reduction.reduction_dimensionality = ReductionDimensionality.TWO_DIM
        state.convert_to_q.reduction_dimensionality = ReductionDimensionality.TWO_DIM
        state.data = data_info

        state.compatibility.use_compatibility_mode = True

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
        )
        output_workspace = single_reduction_alg.getProperty("OutputWorkspaceLAB").value

        # Compare the output of the reduction with the reference
        reference_file_name = "SANS2D_ws_D20_reference_LAB_2D.nxs"
        self._compare_to_reference(
            output_workspace, reference_file_name, mismatch_name=MantidSystemTest.mismatchWorkspaceName(reference_file_name)
        )


# ----------------------------------------------------------------------------------------------------------------------
# Test version 2 of SANSSingleReduction, and compare it to version 1
# ----------------------------------------------------------------------------------------------------------------------
@unittest.skip(
    "Wavelength loops works required a big restructuring for Version 1 but not 2. The SANS"
    " group currently does not use version 2 at all. So disable this test until we are"
    " ready to invest additional time into getting rid of compatibility mode and moving everyone"
    " over."
)
class SANSSingleReduction2Test(SingleReductionTest):
    def __init__(self, *args):
        super(SANSSingleReduction2Test, self).__init__(*args)

    def _assert_group_workspace(self, workspace, n=2):
        """
        Check that a workspace is not None and that it contains n workspaces
        """
        self.assertNotEqual(workspace, None)
        self.assertEqual(workspace.size(), n)

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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        # Set the reduction mode to HAB
        state.reduction.reduction_mode = ReductionMode.HAB
        state.compatibility.use_compatibility_mode = False
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.data = data_info

        # Add some event slices
        state.slice.start_time = [0.00, 300.00]
        state.slice.end_time = [300.00, 600.00]

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceHAB": EMPTY_NAME}
        single_reduction_v2_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=True,
            save_can=True,
            use_optimizations=True,
        )

        # Check output workspaces
        output_workspace = single_reduction_v2_alg.getProperty("OutputWorkspaceHAB").value
        hab_can = single_reduction_v2_alg.getProperty("OutputWorkspaceHABCan").value
        hab_sample = single_reduction_v2_alg.getProperty("OutputWorkspaceHABSample").value
        hab_can_count = single_reduction_v2_alg.getProperty("OutputWorkspaceHABCanCount").value
        hab_can_norm = single_reduction_v2_alg.getProperty("OutputWorkspaceHABCanNorm").value

        self._assert_group_workspace(output_workspace)
        self._assert_group_workspace(hab_can)
        self._assert_group_workspace(hab_sample)
        self._assert_group_workspace(hab_can_count)
        self._assert_group_workspace(hab_can_norm)

        # ---------------------------------------------------
        # Comparison test with version 1
        # This can be removed once version 2 has been adopted
        # ---------------------------------------------------

        # Run the first event slice
        state.slice.start_time = [0.00]
        state.slice.end_time = [300.00]

        single_reduction_alg_first_slice = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=False,
            save_can=True,
        )

        # Run the second event slice
        state.slice.start_time = [300.00]
        state.slice.end_time = [600.00]

        single_reduction_alg_second_slice = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=False,
            save_can=True,
        )
        # Now compare output workspaces from the two versions
        # Output HAB workspace
        event_slice_output_workspace = single_reduction_v2_alg.getProperty("OutputWorkspaceHAB").value
        first_slice_output_workspace = single_reduction_alg_first_slice.getProperty("OutputWorkspaceHAB").value
        second_slice_output_workspace = single_reduction_alg_second_slice.getProperty("OutputWorkspaceHAB").value

        self._compare_workspace(event_slice_output_workspace[0], first_slice_output_workspace, tolerance=1e-6)
        self._compare_workspace(event_slice_output_workspace[1], second_slice_output_workspace, tolerance=1e-6)

        # HAB sample
        event_slice_output_sample = single_reduction_v2_alg.getProperty("OutputWorkspaceHABSample").value
        first_slice_output_sample = single_reduction_alg_first_slice.getProperty("OutputWorkspaceHABSample").value
        second_slice_output_sample = single_reduction_alg_second_slice.getProperty("OutputWorkspaceHABSample").value

        self._compare_workspace(event_slice_output_sample[0], first_slice_output_sample, tolerance=1e-6)
        self._compare_workspace(event_slice_output_sample[1], second_slice_output_sample, tolerance=1e-6)

        # HAB can
        event_slice_output_can = single_reduction_v2_alg.getProperty("OutputWorkspaceHABCan").value
        first_slice_output_can = single_reduction_alg_first_slice.getProperty("OutputWorkspaceHABCan").value
        second_slice_output_can = single_reduction_alg_second_slice.getProperty("OutputWorkspaceHABCan").value

        self._compare_workspace(event_slice_output_can[0], first_slice_output_can, tolerance=1e-6)
        self._compare_workspace(event_slice_output_can[1], second_slice_output_can, tolerance=1e-6)

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

        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.reduction.reduction_mode = ReductionMode.LAB
        state.compatibility.use_compatibility_mode = False
        state.data = data_info

        # Add some event slices
        state.slice.start_time = [0.00, 300.00]
        state.slice.end_time = [300.00, 600.00]

        # Load the sample workspaces
        (
            sample,
            sample_monitor,
            transmission_workspace,
            direct_workspace,
            can,
            can_monitor,
            can_transmission,
            can_direct,
        ) = self._load_workspace(state)

        # Act
        output_settings = {"OutputWorkspaceLAB": EMPTY_NAME}
        single_reduction_v2_alg = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=True,
            save_can=True,
            use_optimizations=True,
        )

        # Check output workspaces
        output_workspace = single_reduction_v2_alg.getProperty("OutputWorkspaceLAB").value
        lab_can = single_reduction_v2_alg.getProperty("OutputWorkspaceLABCan").value
        lab_sample = single_reduction_v2_alg.getProperty("OutputWorkspaceLABSample").value
        lab_can_count = single_reduction_v2_alg.getProperty("OutputWorkspaceLABCanCount").value
        lab_can_norm = single_reduction_v2_alg.getProperty("OutputWorkspaceLABCanNorm").value

        self._assert_group_workspace(output_workspace)
        self._assert_group_workspace(lab_can)
        self._assert_group_workspace(lab_sample)
        self._assert_group_workspace(lab_can_count)
        self._assert_group_workspace(lab_can_norm)

        # ---------------------------------------------------
        # Comparison test with version 1
        # This can be removed once version 2 has been adopted
        # ---------------------------------------------------
        # Run the first event slice
        state.slice.start_time = [0.00]
        state.slice.end_time = [300.00]

        single_reduction_alg_first_slice = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=False,
            save_can=True,
        )

        # Run the second event slice
        state.slice.start_time = [300.00]
        state.slice.end_time = [600.00]

        single_reduction_alg_second_slice = self._run_single_reduction(
            state,
            sample_scatter=sample,
            sample_transmission=transmission_workspace,
            sample_direct=direct_workspace,
            sample_monitor=sample_monitor,
            can_scatter=can,
            can_monitor=can_monitor,
            can_transmission=can_transmission,
            can_direct=can_direct,
            output_settings=output_settings,
            event_slice_optimisation=False,
            save_can=True,
        )
        # Now compare output workspaces from the two versions
        # Output LAB workspace
        event_slice_output_workspace = single_reduction_v2_alg.getProperty("OutputWorkspaceLAB").value
        first_slice_output_workspace = single_reduction_alg_first_slice.getProperty("OutputWorkspaceLAB").value
        second_slice_output_workspace = single_reduction_alg_second_slice.getProperty("OutputWorkspaceLAB").value

        self._compare_workspace(event_slice_output_workspace[0], first_slice_output_workspace, tolerance=1e-6)
        self._compare_workspace(event_slice_output_workspace[1], second_slice_output_workspace, tolerance=1e-6)

        # LAB sample
        event_slice_output_sample = single_reduction_v2_alg.getProperty("OutputWorkspaceLABSample").value
        first_slice_output_sample = single_reduction_alg_first_slice.getProperty("OutputWorkspaceLABSample").value
        second_slice_output_sample = single_reduction_alg_second_slice.getProperty("OutputWorkspaceLABSample").value

        self._compare_workspace(event_slice_output_sample[0], first_slice_output_sample, tolerance=1e-6)
        self._compare_workspace(event_slice_output_sample[1], second_slice_output_sample, tolerance=1e-6)

        # LAB can
        event_slice_output_can = single_reduction_v2_alg.getProperty("OutputWorkspaceLABCan").value
        first_slice_output_can = single_reduction_alg_first_slice.getProperty("OutputWorkspaceLABCan").value
        second_slice_output_can = single_reduction_alg_second_slice.getProperty("OutputWorkspaceLABCan").value

        self._compare_workspace(event_slice_output_can[0], first_slice_output_can, tolerance=1e-6)
        self._compare_workspace(event_slice_output_can[1], second_slice_output_can, tolerance=1e-6)


class SANSReductionRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSSingleReductionTest, "test"))
        suite.addTest(unittest.makeSuite(SANSSingleReduction2Test, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
