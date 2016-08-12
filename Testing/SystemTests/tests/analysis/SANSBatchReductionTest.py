# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import os
import stresstesting

import mantid
from SANS2.UserFile.UserFileStateDirector import UserFileStateDirectorISIS
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSEnumerations import SANSFacility
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm


# -----------------------------------------------
# Tests for the SANSBatchReduction algorithm
# -----------------------------------------------
class SANSBatchReductionTest(unittest.TestCase):

    def _run_batch_reduction(self, states, use_optimizations=False):
        batch_reduction_name = "SANSBatchReduction"
        batch_reduction_options = {"SANSStates": states,
                                   "UseOptimizations": use_optimizations}

        batch_reduction_alg = create_unmanaged_algorithm(batch_reduction_name, **batch_reduction_options)

        # Act
        batch_reduction_alg.execute()
        self.assertTrue(batch_reduction_alg.isExecuted())
        return batch_reduction_alg

    def _compare_workspace(self, workspace, reference_file_name):
        # Load the reference file
        load_name = "LoadNexusProcessed"
        load_options = {"Filename": reference_file_name,
                        SANSConstants.output_workspace: SANSConstants.dummy}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reference_workspace = load_alg.getProperty(SANSConstants.output_workspace).value

        # In order to compare the output workspace with the reference, we need to convert it to rebin it so we
        # get a Workspace2D and then perform a bin masking again
        rebin_name = "Rebin"
        rebin_option = {SANSConstants.input_workspace: workspace,
                        SANSConstants.output_workspace: SANSConstants.dummy,
                        "Params": "8000,-0.025,100000",
                        "PreserveEvents": False}

        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_option)
        rebin_alg.execute()
        rebinned = rebin_alg.getProperty(SANSConstants.output_workspace).value

        mask_name = "MaskBins"
        mask_options = {SANSConstants.input_workspace: rebinned,
                        SANSConstants.output_workspace: SANSConstants.dummy,
                        "XMin": 13000.,
                        "XMax": 15750.}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        mask_alg.execute()
        masked = mask_alg.getProperty(SANSConstants.output_workspace).value

        # Save the workspace out and reload it again. This makes equalizes it with the reference workspace
        f_name = os.path.join(mantid.config.getString('defaultsave.directory'),
                              'SANS_temp_batch_reduction_testout.nxs')

        save_name = "SaveNexus"
        save_options = {"Filename": f_name,
                        "InputWorkspace": masked}
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.execute()
        load_alg.setProperty("Filename", f_name)
        load_alg.setProperty(SANSConstants.output_workspace, SANSConstants.dummy)
        load_alg.execute()
        ws = load_alg.getProperty(SANSConstants.output_workspace).value

        # Compare reference file with the output_workspace
        # We need to disable the instrument comparison, it takes way too long
        # We need to disable the sample -- Not clear why yet
        # operation how many entries can be found in the sample logs
        compare_name = "CompareWorkspaces"
        compare_options = {"Workspace1": ws,
                           "Workspace2": reference_workspace,
                           "Tolerance": 1e-7,
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

    def test_that_batch_reduction_evaluates_LAB(self):
        # Arrange
        # Build the data information
        data_builder = get_data_builder(SANSFacility.ISIS)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_builder.set_calibration("TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        data_info = data_builder.build()

        # Get the rest of the state from the user file
        user_file_director = UserFileStateDirectorISIS(data_info)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        state = user_file_director.construct()

        # Act
        states = {"1": state.property_manager}
        batch_reduction_alg = self._run_batch_reduction(states, use_optimizations=False)

        self.assertTrue(batch_reduction_alg.getProperty("NumberOfOutputWorkspacesLAB").value == 1)
        self.assertTrue(batch_reduction_alg.getProperty("NumberOfOutputWorkspacesHAB").value == 0)
        self.assertTrue(batch_reduction_alg.getProperty("NumberOfOutputWorkspacesMerged").value == 0)

        output_workspace = batch_reduction_alg.getProperty("OutputWorkspaceLAB_1").value

        # Evaluate it up to a defined point
        reference_file_name = "SANS2D_ws_D20_reference_after_masking"
        self._compare_workspace(output_workspace, reference_file_name)


class SANSReductionCoreRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSBatchReductionTest, 'test'))
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
