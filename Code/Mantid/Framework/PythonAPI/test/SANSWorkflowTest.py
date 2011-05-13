import unittest

from MantidFramework import mtd
mtd.initialise()

class PythonAlgorithmTest(unittest.TestCase):
    """
        Tests SANS workflow algorithms
    """
    
    def setUp(self):
        
        self.test_ws = "sans_workflow_ws"
        if mtd.workspaceExists(self.test_ws):
            mtd.deleteWorkspace(self.test_ws)
        
        CreateWorkspace(self.test_ws, DataX=[0.1,0.2,0.3], DataY=[1.0,1.0,1.0], DataE=[0.1,0.1,0.1], NSpec=1, UnitX="MomentumTransfer") 
        LoadInstrument(Filename="BIOSANS_Definition.xml", Workspace=self.test_ws)
        mantid[self.test_ws].getRun().addProperty_dbl("wavelength", 6.0, True)
        mantid[self.test_ws].getRun().addProperty_dbl("wavelength-spread", 0.1, True)
        mantid[self.test_ws].getRun().addProperty_dbl("source-aperture-diameter", 40, True)
        mantid[self.test_ws].getRun().addProperty_dbl("sample-aperture-diameter", 40, True)
        mantid[self.test_ws].getRun().addProperty_dbl("source-sample-distance", 11000, True)
        mantid[self.test_ws].getRun().addProperty_dbl("sample_detector_distance", 6000, True)        
        self.assertEqual(mantid[self.test_ws].dataDx(0)[0], 0)
        self.assertEqual(mantid[self.test_ws].dataDx(0)[1], 0)
        self.assertEqual(mantid[self.test_ws].dataDx(0)[2], 0)
        
    def test_reactor_sans_resolution(self):
        ReactorSANSResolution(InputWorkspace=self.test_ws, OutputWorkspace=self.test_ws)
        
        self.assertAlmostEqual(mantid[self.test_ws].dataDx(0)[0], 0.0453773, 4)
        self.assertAlmostEqual(mantid[self.test_ws].dataDx(0)[1], 0.0840184, 4)
        self.assertAlmostEqual(mantid[self.test_ws].dataDx(0)[2], 0.124066, 4)

        if mtd.workspaceExists(self.test_ws):
            mtd.deleteWorkspace(self.test_ws)

    def test_reactor_sans_resolution_diff_output(self):
        alt_ws = "_test_alt_output"
        ReactorSANSResolution(InputWorkspace=self.test_ws, OutputWorkspace=alt_ws)
        
        self.assertAlmostEqual(mantid[alt_ws].dataDx(0)[0], 0.0453773, 4)
        self.assertAlmostEqual(mantid[alt_ws].dataDx(0)[1], 0.0840184, 4)
        self.assertAlmostEqual(mantid[alt_ws].dataDx(0)[2], 0.124066, 4)

        if mtd.workspaceExists(self.test_ws):
            mtd.deleteWorkspace(self.test_ws)
        if mtd.workspaceExists(alt_ws):
            mtd.deleteWorkspace(alt_ws)

if __name__ == '__main__':
    unittest.main()