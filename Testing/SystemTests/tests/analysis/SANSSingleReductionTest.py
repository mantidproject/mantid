# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import unittest
import stresstesting

#
# from mantid.api import AlgorithmManager
# from mantid.kernel import (Quat, V3D)
# from SANS.Move.SANSMoveWorkspaces import (SANSMoveFactory, SANSMoveLOQ, SANSMoveSANS2D, SANSMoveLARMORNewStyle,
#                                           SANSMoveLARMOROldStyle)
# from SANS2.State.StateBuilder.SANSStateMoveBuilder import get_move_builder
# from SANS2.Common.SANSConstants import SANSConstants
# # Not clear why the names in the module are not found by Pylint, but it seems to get confused. Hence this check
# # needs to be disabled here.
# # pylint: disable=no-name-in-module
# from SANS2.State.SANSStateData import SANSStateDataISIS
# from SANS2.State.SANSState import SANSStateISIS
# from SANS2.State.SANSStateReduction import SANSStateReductionISIS
# from SANS2.Common.SANSEnumerations import (ISISReductionMode, ReductionDimensionality, FitModeForMerge)
#
#
# def load_workspace(file_name):
#     alg = AlgorithmManager.createUnmanaged("Load")
#     alg.initialize()
#     alg.setChild(True)
#     alg.setProperty("Filename", file_name)
#     alg.setProperty("OutputWorkspace", "dummy")
#     alg.execute()
#     return alg.getProperty("OutputWorkspace").value
#
#
# class SANSSingleReductionTest(unittest.TestCase):
#     def test_that_single_reduction_can_be_executed(self):
#         pass
#
#
# class SANSSingleReductionRunnerTest(stresstesting.MantidStressTest):
#     def __init__(self):
#         stresstesting.MantidStressTest.__init__(self)
#         self._success = False
#
#     def runTest(self):
#         suite = unittest.TestSuite()
#         suite.addTest(unittest.makeSuite(SANSSingleReductionTest, 'test'))
#         runner = unittest.TextTestRunner()
#         res = runner.run(suite)
#         if res.wasSuccessful():
#             self._success = True
#
#     def requiredMemoryMB(self):
#         return 2000
#
#     def validate(self):
#         return self._success
#

if __name__ == '__main__':
    unittest.main()
