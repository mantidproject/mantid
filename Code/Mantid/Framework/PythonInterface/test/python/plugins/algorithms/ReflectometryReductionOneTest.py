import unittest
from mantid.simpleapi import *


class ReflectometryReductionOneTest(unittest.TestCase):
    
    
    def setUp(self):
        pass
        
    def tearDown(self):
        pass
    
    def test_check_input_workpace_type(self):
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0], DataY=[0,0,0], NSpec=1)
        self.assertRaises(ValueError, ReflectometryReductionOne, InputWorkspace=not_tof)
    
 
if __name__ == '__main__':
    unittest.main()
