import unittest
from CreateTransmissionWorkspaceTest import CreateTransmissionWorkspaceTest
from mantid.simpleapi import *

from CreateTransmissionWorkspaceBaseTest import CreateTransmissionWorkspaceBaseTest

class CreateTransmissionWorkspaceAutoTest(unittest.TestCase, CreateTransmissionWorkspaceBaseTest):
    
    def algorithm_type(self):
        return CreateTransmissionWorkspaceAuto
    
    def setUp(self):
        CreateTransmissionWorkspaceBaseTest.setUp(self)
        
    def tearDown(self):
        CreateTransmissionWorkspaceBaseTest.setUp(self)
    
    def __init__(self, *args, **kwargs):
        super(CreateTransmissionWorkspaceAutoTest,self).__init__(*args, **kwargs)
    
    def test_minimal(self):

        trans1 = Load('INTER00013463.nxs', OutputWorkspace="trans1")
        
        inst = trans1.getInstrument()
        
        out_ws = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, AnalysisMode="PointDetectorAnalysis")
        history = out_ws.getHistory()
        algHist = history.getAlgorithmHistory(history.size()-1)
        alg = algHist.getChildAlgorithm(0)
        
        '''
        Here we are checking that the applied values (passed to CreateTransmissionWorkspace come from the instrument parameters.
        '''
        self.assertEqual(inst.getNumberParameter("LambdaMin")[0], alg.getProperty("WavelengthMin").value)
        self.assertEqual(inst.getNumberParameter("LambdaMax")[0], alg.getProperty("WavelengthMax").value)
        self.assertEqual(inst.getNumberParameter("MonitorBackgroundMin")[0], alg.getProperty("MonitorBackgroundWavelengthMin").value)
        self.assertEqual(inst.getNumberParameter("MonitorBackgroundMax")[0], alg.getProperty("MonitorBackgroundWavelengthMax").value)
        self.assertEqual(inst.getNumberParameter("MonitorIntegralMin")[0], alg.getProperty("MonitorIntegrationWavelengthMin").value)
        self.assertEqual(inst.getNumberParameter("MonitorIntegralMax")[0], alg.getProperty("MonitorIntegrationWavelengthMax").value)
        self.assertEqual(inst.getNumberParameter("I0MonitorIndex")[0], alg.getProperty("I0MonitorIndex").value)
        self.assertEqual(inst.getNumberParameter("PointDetectorStart")[0], float(alg.getProperty("ProcessingInstructions").value.split(',')[0]))
        self.assertEqual(inst.getNumberParameter("PointDetectorStop")[0], float(alg.getProperty("ProcessingInstructions").value.split(',')[1]))
    
    
 
if __name__ == '__main__':
    unittest.main()