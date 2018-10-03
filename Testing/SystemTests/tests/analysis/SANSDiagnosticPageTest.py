# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import unittest
import os
import stresstesting

import mantid

from sans.state.data import get_data_builder
from sans.common.enums import (DetectorType, SANSFacility, IntegralEnum)
from sans.user_file.state_director import StateDirectorISIS
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm
from sans.gui_logic.models.diagnostics_page_model import run_integral
from sans.common.file_information import SANSFileInformationFactory


# -----------------------------------------------
# Tests for the SANSDiagnosticPage
# -----------------------------------------------
class SANSDiagnosticPageTest(unittest.TestCase):
    def _compare_workspace(self, workspace, reference_file_name):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name,
                        "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reference_workspace = load_alg.getProperty("OutputWorkspace").value

        # Save the workspace out and reload it again. This equalizes it with the reference workspace
        f_name = os.path.join(mantid.config.getString('defaultsave.directory'),
                              'SANS_temp_single_core_reduction_testout.nxs')

        save_name = "SaveNexus"
        save_options = {"Filename": f_name,
                        "InputWorkspace": workspace}
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
        compare_options = {"Workspace1": ws,
                           "Workspace2": reference_workspace,
                           "Tolerance": 1e-6,
                           "CheckInstrument": False,
                           "CheckSample": False,
                           "ToleranceRelErr": True,
                           "CheckAllData": True,
                           "CheckMasking": True,
                           "CheckType": True,
                           "CheckAxes": True,
                           "CheckSpectraMap": True}
        compare_alg = create_unmanaged_algorithm(compare_name, **compare_options)
        compare_alg.setChild(False)
        compare_alg.execute()
        result = compare_alg.getProperty("Result").value
        self.assertTrue(result)

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
        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_state = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_state, file_information)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN -- Remove when appropriate
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # Since we are dealing with event based data but we want to compare it with histogram data from the
        # old reduction system we need to enable the compatibility mode
        user_file_director.set_compatibility_builder_use_compatibility_mode(True)
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        # Construct the final state
        state = user_file_director.construct()

        # Act
        output_workspaces = run_integral('', True, IntegralEnum.Horizontal, DetectorType.LAB, state)

        # Evaluate it up to a defined point
        reference_file_name = "SANS2D_ws_diagnostic_reference.nxs"
        self._compare_workspace(output_workspaces[0], reference_file_name)

    def test_that_produces_correct_workspace_multiperiod_LARMOR(self):
        # Arrange
        # Build the data information
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information("LARMOR00013065")
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("LARMOR00013065")
        data_builder.set_calibration("80tubeCalibration_1-05-2015_r3157-3160.nxs")
        data_state = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = StateDirectorISIS(data_state, file_information)
        user_file_director.set_user_file("USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt")

        # Construct the final state
        state = user_file_director.construct()

        # Act
        output_workspaces = run_integral('', True, IntegralEnum.Horizontal, DetectorType.LAB, state)

        # Evaluate it up to a defined point
        reference_file_name = "LARMOR_ws_diagnostic_reference.nxs"
        self._compare_workspace(output_workspaces[0], reference_file_name)


class SANSDiagnosticPageRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSDiagnosticPageTest, 'test'))
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
