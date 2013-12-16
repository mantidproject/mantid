import unittest
from testhelpers.algorithm_decorator import make_decorator
import mantid.api
from CreateTransmissionWorkspaceTest import CreateTransmissionWorkspaceTest
from mantid.simpleapi import *

class CreateTransmissionWorkspaceAutoTest(CreateTransmissionWorkspaceTest):
    
    def __init__(self, *args, **kwargs):
        super(CreateTransmissionWorkspaceAutoTest, self).__init__(*args, **kwargs)
        trans1 = Load('INTER00013463.nxs', OutputWorkspace="trans1")
        trans2 = Load('INTER00013464.nxs', OutputWorkspace="trans2")
        self.__trans1 = trans1
        self.__trans2 = trans2
        
    def setUp(self):
        super(CreateTransmissionWorkspaceAutoTest, self).setUp()
        standard_alg = make_decorator(self.algorithm_type())
        self.__standard_alg = standard_alg
        
    def __del__(self):
        pass
        #DeleteWorkspace(self.__trans1)
        #DeleteWorkspace(self.__trans2)
    
    
    def algorithm_type(self):
        return CreateTransmissionWorkspaceAuto
    
    def test_use_point_detector_start(self):
        alg = self.__standard_alg
        print self.__trans1
        alg.set_FirstTransmissionRun(self.__trans1)
        alg.set_AnalysisMode("PointDetectorAnalysis")
        alg.execute()
        
    
    
    
 
if __name__ == '__main__':
    unittest.main()