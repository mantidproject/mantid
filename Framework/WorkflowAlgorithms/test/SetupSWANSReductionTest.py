import unittest
from mantid.simpleapi import *

import mantid
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
config = ConfigService.Instance()
config['instrumentName']='SWANS'

class SetupSWANSReductionTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_init(self):
        SWANS()
        SolidAngle(detector_tubes=True)
        SetWavelengthStep(0.1)
        Reduce()

if __name__ == '__main__':
    unittest.main()