# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import stresstesting

import mantid
from mantid.api import AlgorithmManager
from SANS2.UserFile.UserFileStateDirector import UserFileStateDirectorISIS
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSEnumerations import SANSFacility
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm


# -----------------------------------------------
# Tests for the SANSReductionCore algorithm
# -----------------------------------------------
class SANSReductionCoreTest(unittest.TestCase):
    def _load_workspace(self, state):
        load_alg = AlgorithmManager.createUnmanaged("SANSLoad")
        load_alg.setChild(True)
        load_alg.initialize()

        state_dict = state.property_manager
        load_alg.setProperty("SANSState", state_dict)
        load_alg.setProperty("PublishToCache", False)
        load_alg.setProperty("UseCached", False)
        load_alg.setProperty("MoveWorkspace", False)

        # Act
        load_alg.execute()
        self.assertTrue(load_alg.isExecuted())
        sample_scatter = load_alg.getProperty("SampleScatterWorkspace").value
        sample_scatter_monitor_workspace = load_alg.getProperty("SampleScatterMonitorWorkspace").value
        return sample_scatter, sample_scatter_monitor_workspace

    def _run_reduction_core(self, state, workspace, monitor, transmission=None, direct=None, detector_type="LAB"):
        reduction_core_alg = AlgorithmManager.createUnmanaged("SANSReductionCore")
        reduction_core_alg.setChild(True)
        reduction_core_alg.initialize()

        state_dict = state.property_manager
        reduction_core_alg.setProperty("SANSState", state_dict)
        reduction_core_alg.setProperty("ScatterWorkspace", workspace)
        reduction_core_alg.setProperty("ScatterMonitorWorkspace", monitor)

        if transmission:
            reduction_core_alg.setProperty("TransmissionWorkspace", transmission)

        if direct:
            reduction_core_alg.setProperty("DirectWorkspace", direct)

        if detector_type == "HAB":
            reduction_core_alg.setProperty("Component", "HAB")
        else:
            reduction_core_alg.setProperty("Component", "LAB")

        reduction_core_alg.setProperty(SANSConstants.output_workspace, SANSConstants.dummy)

        # Act
        reduction_core_alg.execute()
        self.assertTrue(reduction_core_alg.isExecuted())
        return reduction_core_alg

    def test_that_reduction_core_evaluates_LAB(self):
        # Arrange
        data_builder = get_data_builder(SANSFacility.ISIS)
        data_builder.set_sample_scatter("SANS2D00034484")
        data_builder.set_sample_transmission("SANS2D00034505")
        data_builder.set_sample_direct("SANS2D00034461")
        data_info = data_builder.build()
        user_file_director = UserFileStateDirectorISIS(data_info)
        user_file_director.set_user_file("USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt")
        state = user_file_director.construct()

        workspace, workspace_monitor = self._load_workspace(state)

        # Act
        reduction_core_alg = self._run_reduction_core(state, workspace, workspace_monitor)
        output_workspace = reduction_core_alg.getProperty(SANSConstants.output_workspace).value
        save_name = "SaveNexus"
        save_options = {"InputWorkspace": output_workspace,
                        "Filename": "C:/Users/pica/Desktop/core2.nxs"}
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.execute()

    # def test_that_reduction_core_evaluates_HAB(self):
    #     # Arrange
    #     data_builder = get_data_builder(SANSFacility.ISIS)
    #     data_builder.set_sample_scatter("SANS2D00028827")
    #     data_info = data_builder.build()
    #     user_file_director = UserFileStateDirectorISIS(data_info)
    #     state = user_file_director.construct()
    #
    #     workspace, workspace_monitor = self._load_workspace(state)
    #
    #     # Act
    #     reduction_core_alg = self._run_reduction_core(state, workspace, workspace_monitor, detector_type="HAB")
    #     output_workspace = reduction_core_alg.getProperty(SANSConstants.output_workspace).value
    #     print output_workspace


class SANSReductionCoreRunnerTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSReductionCoreTest, 'test'))
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
