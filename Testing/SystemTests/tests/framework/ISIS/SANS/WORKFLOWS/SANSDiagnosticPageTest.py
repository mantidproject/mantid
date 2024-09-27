# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import os
from unittest import mock

import systemtesting

import mantid
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantidqtinterfaces.sans_isis.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans_core.state.StateObjects.StateData import get_data_builder
from sans_core.common.enums import DetectorType, SANSFacility, IntegralEnum, SANSInstrument

from sans_core.common.constants import EMPTY_NAME
from sans_core.common.general_functions import create_unmanaged_algorithm
from mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.diagnostic_async import DiagnosticsAsync
from sans_core.common.file_information import SANSFileInformationFactory


# -----------------------------------------------
# Tests for the SANSDiagnosticPage
# -----------------------------------------------
from sans_core.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class SANSDiagnosticPageTest(unittest.TestCase):
    def _compare_workspace(self, workspace, reference_file_name):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reference_workspace = load_alg.getProperty("OutputWorkspace").value

        # Save the workspace out and reload it again. This equalizes it with the reference workspace
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
        }
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value
        message = compare_alg.getProperty("Messages").value
        self.assertTrue(result, message)

        # Remove file
        if os.path.exists(f_name):
            os.remove(f_name)

    def test_that_produces_correct_workspace_for_SANS2D(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("SANS2D00034484")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_state = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.compatibility.use_compatibility_mode = True
        state.adjustment.calibration = "TUBE_SANS2D_BOTH_31681_25Sept15.nxs"
        state.data = data_state

        # Act
        mocked_parent = mock.create_autospec(DiagnosticsPagePresenter)
        async_worker = DiagnosticsAsync(parent_presenter=mocked_parent)
        async_worker.set_unit_test_mode(True)
        async_worker.run_integral("", True, IntegralEnum.Horizontal, DetectorType.LAB, state)
        mocked_parent.on_processing_success.assert_called_once()
        mocked_parent.on_processing_finished.assert_called_once()
        output_workspaces = mocked_parent.on_processing_success.call_args_list[0][1]["output"]

        self.assertEqual(len(output_workspaces), 1)
        # Evaluate it up to a defined point
        reference_file_name = "SANS2D_ws_centred_diagnostic_reference.nxs"
        self._compare_workspace(output_workspaces[0], reference_file_name)

    def test_that_produces_correct_workspace_multiperiod_LARMOR(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("LARMOR00013065")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("LARMOR00013065")

        data_state = data_builder.build()

        # Get the rest of the state from the user file
        user_file = "USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt"
        user_file_director = UserFileReaderAdapter(file_information=file_information, user_file_name=user_file)
        state = user_file_director.get_all_states(file_information=file_information)
        state.adjustment.calibration = "80tubeCalibration_1-05-2015_r3157-3160.nxs"
        state.data = data_state

        # Act
        mocked_parent = mock.create_autospec(DiagnosticsPagePresenter)
        async_worker = DiagnosticsAsync(parent_presenter=mocked_parent)
        async_worker.set_unit_test_mode(True)
        async_worker.run_integral("", True, IntegralEnum.Horizontal, DetectorType.LAB, state)

        mocked_parent.on_processing_success.assert_called_once()
        mocked_parent.on_processing_finished.assert_called_once()
        output_workspaces = mocked_parent.on_processing_success.call_args_list[0][1]["output"]

        # Evaluate it up to a defined point
        reference_file_name = "LARMOR_ws_diagnostic_reference.nxs"
        self._compare_workspace(output_workspaces[0], reference_file_name)
        mocked_parent.on_processing_finished.assert_called_once()


@ISISSansSystemTest(SANSInstrument.LARMOR, SANSInstrument.SANS2D)
class SANSDiagnosticPageRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSDiagnosticPageTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
