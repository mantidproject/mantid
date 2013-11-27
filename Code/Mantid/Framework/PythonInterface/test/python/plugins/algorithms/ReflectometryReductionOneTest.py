import unittest
from mantid.simpleapi import *


class ReflectometryReductionOneTest(unittest.TestCase):
    
    
    def setUp(self):
        tof = CreateWorkspace(UnitX="TOF", DataX=[0,0,0], DataY=[0,0,0], NSpec=1)
        not_tof =  CreateWorkspace(UnitX="1/q", DataX=[0,0,0], DataY=[0,0,0], NSpec=1)
        self.__tof = tof
        self.__not_tof = not_tof
        
    def tearDown(self):
        DeleteWorkspace(self.__tof)
        DeleteWorkspace(self.__not_tof)
    
    def test_check_input_workpace_not_tof_throws(self):
        self.assertRaises(ValueError, ReflectometryReductionOne, InputWorkspace=self.__not_tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__tof)
    
    def test_check_first_transmission_workspace_not_tof_throws(self):
        self.assertRaises(ValueError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__not_tof, SecondTransmissionRun=self.__tof)
    
    def test_check_second_transmission_workspace_not_tof_throws(self):
        self.assertRaises(ValueError, ReflectometryReductionOne, InputWorkspace=self.__tof, FirstTransmissionRun=self.__tof, SecondTransmissionRun=self.__not_tof)

if __name__ == '__main__':
    unittest.main()
