# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import os
import systemtesting

import mantid
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.api import AlgorithmManager
from sans.state.Serializer import Serializer

from sans.state.StateObjects.StateData import get_data_builder
from sans.common.enums import DetectorType, DataType, SANSFacility, SANSInstrument

from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.file_information import SANSFileInformationFactory


# -----------------------------------------------
# Tests for the SANSReductionCore algorithm
# -----------------------------------------------
from sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSReductionCoreTest(unittest.TestCase):
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
        if state.data.sample_transmission:
            load_alg.setProperty("SampleTransmissionWorkspace", EMPTY_NAME)
        if state.data.sample_direct:
            load_alg.setProperty("SampleDirectWorkspace", EMPTY_NAME)

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        sample_scatter = load_alg.getProperty("SampleScatterWorkspace").value
        sample_scatter_monitor_workspace = load_alg.getProperty("SampleScatterMonitorWorkspace").value
        if state.data.sample_transmission:
            transmission_workspace = load_alg.getProperty("SampleTransmissionWorkspace").value
        else:
            transmission_workspace = None
        if state.data.sample_direct:
            direct_workspace = load_alg.getProperty("SampleDirectWorkspace").value
        else:
            direct_workspace = None
        return sample_scatter, sample_scatter_monitor_workspace, transmission_workspace, direct_workspace

    def _run_reduction_core(
        self, state, workspace, monitor, transmission=None, direct=None, detector_type=DetectorType.LAB, component=DataType.SAMPLE
    ):
        reduction_core_alg = AlgorithmManager.createUnmanaged("SANSReductionCore")
        reduction_core_alg.setChild(True)
        reduction_core_alg.initialize()

        state_dict = Serializer.to_json(state)
        reduction_core_alg.setProperty("SANSState", state_dict)
        reduction_core_alg.setProperty("ScatterWorkspace", workspace)
        reduction_core_alg.setProperty("ScatterMonitorWorkspace", monitor)

        if transmission:
            reduction_core_alg.setProperty("TransmissionWorkspace", transmission)

        if direct:
            reduction_core_alg.setProperty("DirectWorkspace", direct)

        reduction_core_alg.setProperty("Component", detector_type.value)
        reduction_core_alg.setProperty("DataType", component.value)

        reduction_core_alg.setProperty("OutputWorkspaces", EMPTY_NAME)

        reduction_core_alg.setProperty("CalculatedTransmissionWorkspaces", EMPTY_NAME)
        reduction_core_alg.setProperty("UnfittedTransmissionWorkspaces", EMPTY_NAME)

        # Act
        reduction_core_alg.execute()
        self.assertTrue(reduction_core_alg.isExecuted())
        return reduction_core_alg

    def _compare_workspace(self, workspace, reference_file_name):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reference_workspace = load_alg.getProperty("OutputWorkspace").value

        # Save the workspace out and reload it again. This makes equalizes it with the reference workspace
        f_name = os.path.join(mantid.config.getString("defaultsave.directory"), "SANS_temp_single_core_reduction_testout.nxs")

        save_name = "SaveNexus"
        save_options = {"Filename": f_name, "InputWorkspace": workspace}
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.execute()
        load_alg.setProperty("Filename", f_name)
        load_alg.setProperty("OutputWorkspace", EMPTY_NAME)
        load_alg.execute()

        ws = load_alg.getProperty("OutputWorkspace").value

        # Compare reference file with the output_workspace
        # We need to disable the instrument comparison, it takes way too long
        # We need to disable the sample -- since the sample has been modified (more logs are being written)
        # operation how many entries can be found in the sample logs
        compare_name = "CompareWorkspaces"
        compare_options = {
            "Workspace1": ws,
            "Workspace2": reference_workspace,
            "Tolerance": 1e-6,
            "CheckInstrument": False,
            "CheckSample": False,
            "ToleranceRelErr": True,
            "CheckAllData": True,
            "CheckMasking": True,
            "CheckType": True,
            "CheckAxes": True,
            "CheckSpectraMap": True,
            "NaNsEqual": True,
        }
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value
        self.assertTrue(result)

        # Remove file
        if os.path.exists(f_name):
            os.remove(f_name)

    def test_that_reduction_core_evaluates_LAB(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_state = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)

        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.compatibility.use_compatibility_mode = True
        state.data = data_state

        # Load the sample workspaces
        workspace, workspace_monitor, transmission_workspace, direct_workspace = self._load_workspace(state)

        # Act
        reduction_core_alg = self._run_reduction_core(state, workspace, workspace_monitor, transmission_workspace, direct_workspace)
        output_workspace = reduction_core_alg.getProperty("OutputWorkspaces").value
        calculated_transmission = reduction_core_alg.getProperty("CalculatedTransmissionWorkspaces").value
        unfitted_transmission = reduction_core_alg.getProperty("UnfittedTransmissionWorkspaces").value

        # Evaluate it up to a defined point
        reference_file_name = "SANS2D_ws_D20_reference.nxs"
        self._compare_workspace(output_workspace, reference_file_name)

        calculated_transmission_reference_file = "SANS2D_ws_D20_calculated_transmission_reference.nxs"
        unfitted_transmission_reference_file = "SANS2D_ws_D20_unfitted_transmission_reference.nxs"
        self._compare_workspace(calculated_transmission, calculated_transmission_reference_file)
        self._compare_workspace(unfitted_transmission, unfitted_transmission_reference_file)

    def test_similarity_between_results_in_compatibility_mode_and_non_compatibility_mode(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_state = data_builder.build()

        ################################################################################################################
        # Compatibility mode
        ################################################################################################################
        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)

        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.compatibility.use_compatibility_mode = True
        state.data = data_state

        # Load the sample workspaces
        workspace, workspace_monitor, transmission_workspace, direct_workspace = self._load_workspace(state)

        # Act
        reduction_core_alg = self._run_reduction_core(state, workspace, workspace_monitor, transmission_workspace, direct_workspace)
        compatibility_output_workspace = reduction_core_alg.getProperty("OutputWorkspaces").value

        ################################################################################################################
        # Non-compatibility mode
        ################################################################################################################
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.compatibility.use_compatibility_mode = False
        state.data = data_state

        # Load the sample workspaces
        workspace, workspace_monitor, transmission_workspace, direct_workspace = self._load_workspace(state)

        # Act
        reduction_core_alg = self._run_reduction_core(state, workspace, workspace_monitor, transmission_workspace, direct_workspace)
        non_compatibility_output_workspace = reduction_core_alg.getProperty("OutputWorkspaces").value

        ################################################################################################################
        # Compare workspaces
        ################################################################################################################
        compare_name = "CompareWorkspaces"
        compare_options = {
            "Workspace1": non_compatibility_output_workspace.getItem(0),
            "Workspace2": compatibility_output_workspace.getItem(0),
            "Tolerance": 1,
            "CheckInstrument": False,
            "CheckSample": False,
            "ToleranceRelErr": True,
            "CheckAllData": True,
            "CheckMasking": True,
            "CheckType": True,
            "CheckAxes": True,
            "CheckSpectraMap": True,
            "NaNsEqual": True,
        }
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value
        self.assertTrue(result)


class SANSReductionCoreRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSReductionCoreTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
