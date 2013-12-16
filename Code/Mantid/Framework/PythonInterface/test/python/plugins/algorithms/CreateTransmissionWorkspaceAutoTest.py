import unittest
import mantid.api
from mantid.simpleapi import *

class CreateTransmissionWorkspaceAutoTest(unittest.TestCase):
    
    
    def test_input_workspace_not_tof_throws(self):
        inws = CreateWorkspace(DataY=[1,1,1], DataX=[0,1,2,3], UnitX="TOF")
        out = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=inws)
        
            
 
if __name__ == '__main__':
    unittest.main()