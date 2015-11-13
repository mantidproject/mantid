#pylint: disable=no-init
import unittest
import stresstesting
from mantid.simpleapi import *
import ISISCommandInterface as command_iface
from reducer_singleton import ReductionSingleton
from isis_reduction_steps import DarkRunSubtraction

class SANSDarkRunSubtractionBareReductionStepTest(unittest.TestCase):
    def test_that_can_apply_dark_run_to_rear(self):
        print "sdflksdfsdf"
        # Arrange
        command_iface.Clean()
        command_iface.SANS2D()
        command_iface.MaskFile('MASKSANS2D_094i_RKH.txt')
        command_iface.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        command_iface.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        command_iface.Gravity(False)
        command_iface.Set1D()
        command_iface.AssignSample('2500.nxs')

        print "]]]]]]]]]]]]]]]]]]]]]]]]]]"
        for element in mtd.getObjectNames():
            print element

        # Act
        #dark_run_subtraction = DarkRunSubtraction()
        #dark_run_subtraction.execute(
        # Assert



