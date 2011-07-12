import unittest
from mantidsimple import *

class EQSANSQ2DTest(unittest.TestCase):

    def setUp(self):
        
        self.test_ws_name = "EQSANS_test_ws"
        x = [1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11.]
        y = 491520*[0.1]    
        CreateWorkspace(OutputWorkspace=self.test_ws_name,DataX=x,DataY=y,DataE=y,NSpec='49152',UnitX='Wavelength')
        LoadInstrument(self.test_ws_name, InstrumentName="EQSANS")
        mtd[self.test_ws_name].getRun().addProperty_dbl("sample_detector_distance", 4000.0, 'mm', True)
        mtd[self.test_ws_name].getRun().addProperty_dbl("beam_center_x", 96, 'pixel', True)            
        mtd[self.test_ws_name].getRun().addProperty_dbl("beam_center_y", 128, 'pixel', True)   
        mtd[self.test_ws_name].getRun().addProperty_dbl("wavelength_min", 1, "Angstrom", True)
        mtd[self.test_ws_name].getRun().addProperty_dbl("wavelength_max", 11, "Angstrom", True)
        mtd[self.test_ws_name].getRun().addProperty_int("is_frame_skipping", 0, True)
        mtd[self.test_ws_name].getRun().addProperty_dbl("wavelength_min_frame2", 5, "Angstrom", True)
        mtd[self.test_ws_name].getRun().addProperty_dbl("wavelength_max_frame2", 10, "Angstrom", True)

    def tearDown(self):
        if mtd.workspaceExists(self.test_ws_name):
            mtd.deleteWorkspace(self.test_ws_name)          

    def test_q2d(self):
        EQSANSQ2D(self.test_ws_name)
        ReplaceSpecialValues(InputWorkspace=self.test_ws_name+"_Iqxy",OutputWorkspace=self.test_ws_name+"_Iqxy",NaNValue=0,NaNError=0)
        Integration(self.test_ws_name+"_Iqxy", "__tmp")
        SumSpectra("__tmp", "summed")
        self.assertAlmostEquals(mtd["summed"].dataY(0)[0], 7.24077, 6)
        for ws in ["__tmp", "summed"]:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)          
        

if __name__ == '__main__':
    unittest.main()