# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import stresstesting

from mantid.api import AlgorithmManager

from SANS2.Common.SANSEnumerations import SANSFacility
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.UserFile.UserFileStateDirector import UserFileStateDirectorISIS
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm


def get_masked_spectrum_numbers(workspace):
    for index in range(workspace.getNumberHistograms()):
        try:
            det = workspace.getDetector(index)
        except RuntimeError:
            break
        if det.isMasked():
            yield workspace.getSpecturm(index).getSpectrumNo()


# -----------------------------------------------
# Tests for the SANSLoad algorithm
# -----------------------------------------------
class SANSMaskWorkspaceTest(unittest.TestCase):
    def _load_workspace(self, state):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = state.property_manager
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", False)
        load_alg.setProperty("UseCached", False)
        load_alg.setProperty("MoveWorkspace", True)

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        return load_alg.getProperty("SampleScatterWorkspace").value

    def _run_mask(self, state, workspace, component):
        mask_alg = AlgorithmManager.createUnmanaged("SANSMaskWorkspace")
        mask_alg.setChild(True)
        mask_alg.initialize()

        state_dict = state.property_manager
        mask_alg.setProperty("SANSState", state_dict)
        mask_alg.setProperty("Workspace", workspace)
        mask_alg.setProperty("Component", component)

        mask_alg.execute()
        self.assertTrue(mask_alg.isExecuted())
        return mask_alg.getProperty("Workspace").value

    def test_that_masks_SANS2D_workspace_correctly(self):
        # Arrange
        import time
        time.sleep(5)
        print "======================="
        data_builder = get_data_builder(SANSFacility.ISIS)
        data_builder.set_sample_scatter("SANS2D00028827")
        data_info = data_builder.build()

        user_file_director = UserFileStateDirectorISIS(data_info)
        user_file_director.set_user_file("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        state = user_file_director.construct()

        workspace = self._load_workspace(state)

        # Act
        workspace = self._run_mask(state, workspace, "LAB")

        masked_spectra = get_masked_spectrum_numbers(workspace)
        for masked_spectrum in masked_spectra:
            print masked_spectrum

        # from mantid.api import AnalysisDataService
        # AnalysisDataService.add("test_workspace", workspace)
        # save_name = "SaveNexus"
        # save_options = {"Filename": "C:/Users/pica/Desktop/test3.nxs",
        #                 "InputWorkspace": workspace}
        # save_alg = create_unmanaged_algorithm(save_name, **save_options)
        # save_alg.execute()

        # Assert

    def test_that_SANS2D_workspace_can_be_masked_v2(self):
        pass


class SANSLoadDataRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSMaskWorkspaceTest, 'test'))
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
