import unittest
import math
from mantidsimple import *

class EQSANSSensitivityCorrectionTest(unittest.TestCase):

    def setUp(self):
        
        self.test_ws_name = "EQSANS_test_ws"
        self.eff_ws_name = "EQSANS_eff_test_ws"
        x = [1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11.]
        y = 200*[0.1]    
        CreateWorkspace(OutputWorkspace=self.test_ws_name,DataX=x,DataY=y,DataE=y,NSpec='20',UnitX='Wavelength')
        CreateWorkspace(OutputWorkspace=self.eff_ws_name,DataX=x,DataY=y,DataE=y,NSpec='20',UnitX='Wavelength')

    def tearDown(self):
        for ws in [self.test_ws_name, self.eff_ws_name]:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)          

    def test_sensitivity(self):
        EQSANSSensitivityCorrection(InputWorkspace=self.test_ws_name, EfficiencyWorkspace=self.test_ws_name, 
                                    Factor=0.9, Error=0.005, OutputWorkspace=self.test_ws_name,
                                    OutputEfficiencyWorkspace=self.eff_ws_name)
        
        eff0 = mtd[self.eff_ws_name].dataY(0)
        self.assertEquals(len(eff0), 10)
        self.assertEquals(mtd[self.eff_ws_name].getNumberHistograms(), 20)
        self.assertAlmostEquals(eff0[0], 0.1*(1.0-math.exp(-0.9*1.5)), 6)

if __name__ == '__main__':
    unittest.main()