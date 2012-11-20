import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class Stitch1DTest(unittest.TestCase):
          
    def test_stitching(self):
        x = numpy.arange(-1, 1, 0.2)
        e = [1,1,1,1,1,1,1,1,1,1]
        alg_lhs =  run_algorithm("CreateWorkspace", UnitX="1/q", DataX=x, DataY='0,0,0,3,3,3,3,3,3,3', NSpec='1', DataE=e, OutputWorkspace='a')
        alg_rhs =  run_algorithm("CreateWorkspace", UnitX="1/q", DataX=x, DataY='2,2,2,2,2,2,2,0,0,0', NSpec='1', DataE=e, OutputWorkspace='b')
        
        alg_out = run_algorithm("Stitch1D", LHSWorkspace=alg_lhs.getPropertyValue("OutputWorkspace"), RHSWorkspace=alg_rhs.getPropertyValue("OutputWorkspace"), OutputWorkspace="c", StartOverlap=0.3, EndOverlap=0.7, rethrow=True)
        out_ws = mtd[alg_out.getPropertyValue("OutputWorkspace")]
        
        self.assertEquals(1, out_ws.getNumberHistograms())
        output_signal =  out_ws.readY(0)
        
        expected_output_signal =[3,3,3,3,3,3,3,3,3,3]
        
        for i in range(0, len(output_signal)):
            self.assertEqual(round(expected_output_signal[i], 5), round(output_signal[i],5) ) 

            
    def test_calculates_scaling_factor_correctly(self):
        # Signal = 1, 1, 1, but only the middle to the end of the range is marked as overlap, so only 1, 1 used.
        alg_a = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,1,1', NSpec='1', DataE='1,1,1', OutputWorkspace='flat_signal')
        # Signal = 1, 2, 3, but only the middle to the end of the range is marked as overlap, so only 2, 3 used.
        alg_b = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,2,3', NSpec='1', DataE='1,1,1', OutputWorkspace='rising_signal')
   
        alg = run_algorithm("Stitch1D", LHSWorkspace='flat_signal', RHSWorkspace='rising_signal',OutputWorkspace='converted',StartOverlap=0.5,EndOverlap=1,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        
        b_use_manual_scaling = alg.getProperty("UseManualScaleFactor").value
        self.assertEqual(False, b_use_manual_scaling)
        
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        
         # 1 * (( 1 + 1) / (2 + 3)) = 0.4
        self.assertEqual(0.4, scale_factor)
          
    def test_calculates_scaling_factor_correctly_inverted(self):
        # Signal = 1, 1, 1, but only the middle to the end of the range is marked as overlap, so only 1, 1 used.
        alg_lhs = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,1,1', NSpec='1', DataE='1,1,1', OutputWorkspace='flat_signal')
        # Signal = 1, 2, 3, but only the middle to the end of the range is marked as overlap, so only 2, 3 used.
        alg_rhs = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,2,3', NSpec='1', DataE='1,1,1', OutputWorkspace='rising_signal')
   
        alg = run_algorithm("Stitch1D", LHSWorkspace=alg_lhs.getPropertyValue("OutputWorkspace"), RHSWorkspace=alg_rhs.getPropertyValue("OutputWorkspace"),OutputWorkspace='converted',ScaleRHSWorkspace=False,StartOverlap=0.5,EndOverlap=1,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        
        # 1 * ((2 + 3)/( 1 + 1)) = 2.5
        self.assertEqual(2.5, scale_factor)
        
    def test_manual_scaling_factor(self):
    
        alg_a = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,1,1', NSpec='1', DataE='1,1,1', OutputWorkspace='a')
        alg_b = run_algorithm("CreateWorkspace", UnitX="A", DataX='-1,0,1', DataY='1,1,1', NSpec='1', DataE='1,1,1', OutputWorkspace='b')
        
        expected_manual_scale_factor = 2.2
        
        alg = run_algorithm("Stitch1D", LHSWorkspace='a', RHSWorkspace='b',OutputWorkspace='converted',StartOverlap=0.5,EndOverlap=1,ScaleRHSWorkspace=True,UseManualScaleFactor=True,ManualScaleFactor=expected_manual_scale_factor,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        self.assertEqual(expected_manual_scale_factor, scale_factor)
 
if __name__ == '__main__':
    unittest.main()