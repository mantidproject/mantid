"""
    Unit tests for SANS reduction command set
"""
import math
import unittest
from CommandInterface import *
from HFIRtest import _read_IGOR
from mantidsimple import *

# Set directory containg the test data, relative to the Mantid release directory.
TEST_DIR = "../../../Test/Data/SANS2D/"

        
def _check_result(ws, test_file, tolerance=1e-6):
    """
        Compare the data in two reduced data files.
        @param reduced_file: path of the Mantid-reduced file
        @param test_file: path of the IGOR-reduced file
    """
    passed = True
    
    # Read mantid data  
    x = ws.dataX(0)[:len(ws.dataX(0))]
    y = ws.dataY(0)
    e = ws.dataE(0)
    data_mantid = zip(x,y,e)
    
    # Read the test data to compare with
    data_igor = _read_IGOR(test_file)
    
    # Check length
    assert(len(data_mantid)==len(data_igor))
    
    # Utility methods for manipulating the lists
    def _diff_chi2(x,y): return (x[1]-y[1])*(x[1]-y[1])/(x[2]*x[2])
    def _diff_iq(x,y): return x[1]-y[1]
    def _diff_err(x,y): return x[2]-y[2]
    def _add(x,y): return x+y
    
    # Check that I(q) is the same for both data sets
    deltas = map(_diff_iq, data_mantid, data_igor)
    delta  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta)>tolerance:
        passed = False
        print "Sum of I(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta), tolerance)
    
    # Then compare the errors
    deltas = map(_diff_err, data_mantid, data_igor)
    delta_err  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta_err)>tolerance:
        passed = False
        print "Sum of dI(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta_err), tolerance)
    
    # Compute chi2 of our result relative to IGOR 
    deltas = map(_diff_chi2, data_mantid, data_igor)
    chi2  = reduce(_add, deltas)/len(data_igor)
    if chi2>10.0*tolerance:
        passed= False
        print "Chi2 is outside tolerance: %g > %g" % (chi2, 10.0*tolerance)
 
    return passed

class TestCommands(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        pass
                
    def test_data_path(self):
        self.assertEqual(ReductionSingleton()._data_path, '.')
        DataPath("/tmp")
        self.assertEqual(ReductionSingleton()._data_path, '/tmp')
        
    def test_direct_beam_center(self):
        DataPath("../../../Test/Data/SANS2D/")
        HFIRSANS()
        FindBeamCenter("BioSANS_empty_cell.xml", FindBeamCenter.BEAMFINDER_DIRECT_BEAM)
        Reduce1D()
        center = ReductionSingleton()._beam_finder.get_beam_center()
        self.assertAlmostEqual(center[0], 16.6038, 0.0001)
        self.assertAlmostEqual(center[1], 96.771, 0.0001)
        
    def test_hand_beam_center(self):
        HFIRSANS()
        SetBeamCenter(1.1, 2.2)
        center = ReductionSingleton()._beam_finder.get_beam_center()
        self.assertAlmostEqual(center[0], 1.1, 0.0001)
        self.assertAlmostEqual(center[1], 2.2, 0.0001)
        
    def test_load_run(self):
        DataPath("../../../Test/Data/SANS2D/")
        HFIRSANS()
        self.assertEqual(len(ReductionSingleton()._data_files), 0)
        AppendDataFile("BioSANS_test_data.xml")
        self.assertEqual(len(ReductionSingleton()._data_files), 1)      
        
    def test_norm_options(self):
        self.assertEqual(ReductionSingleton()._normalizer._normalization_spectrum, 1)
        MonitorNormalization()
        self.assertEqual(ReductionSingleton()._normalizer._normalization_spectrum, 0)
        TimeNormalization()
        self.assertEqual(ReductionSingleton()._normalizer._normalization_spectrum, 1)
        
    def test_to_steps(self):
        DataPath("../../../Test/Data/SANS2D/")
        HFIRSANS()
        FindBeamCenter("BioSANS_empty_cell.xml", FindBeamCenter.BEAMFINDER_DIRECT_BEAM)
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        SensitivityCorrection("BioSANS_flood_data.xml")
        Reduce1D()
        
        self.assertEqual(ReductionSingleton()._normalizer._normalization_spectrum, 1)
        
        self.assertTrue(_check_result(mtd["Iq"], TEST_DIR+"reduced_center_calculated.txt"))
    
    def skip_test_reduction_1(self):
        DataPath("../../../Test/Data/SANS2D/")
        HFIRSANS()
        FindBeamCenter("BioSANS_empty_cell.xml", FindBeamCenter.BEAMFINDER_DIRECT_BEAM)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml")
        Reduce1D()
        
        data = mtd["Iq"].dataY(0)
        check = [0.19472,0.204269,0.215354,0.230114,0.238961,0.237201,0.247843,0.248424,0.253676,0.254327,0.254366,0.252931,0.258339,0.259297,0.257155,0.254059,0.252383,0.252826,0.256604,0.256754,0.255592,0.256813,0.248569,0.25331,0.251032,0.246424,0.249477,0.250939,0.251959,0.24925,0.250372,0.246148,0.250478,0.244621,0.247428,0.246431,0.245041,0.241647,0.24307,0.240096,0.242797,0.238182,0.237548,0.239789,0.241477,0.23456,0.237372,0.233715,0.233789,0.232262,0.231589,0.230986,0.231646,0.231331,0.230484,0.2277,0.226819,0.224341,0.227239,0.223228,0.221232,0.222011,0.224747,0.219533,0.216973,0.218734,0.21668,0.218366,0.214926,0.213985,0.214469,0.210473,0.209867,0.209066,0.208965,0.207498,0.204505,0.205786,0.202186,0.200442,0.200485,0.200554,0.200499,0.198152,0.193945,0.192082,0.193783,0.193787,0.190557,0.190471,0.186827,0.190088,0.188204,0.187547,0.182206,0.181384,0.180358,0.182663,0.178844,0.176556]

        for i in range(len(check)):
            self.assertAlmostEqual(data[i], check[i], 0.00001)

    def skip_test_reduction_2(self):
        DataPath("../../../Test/Data/SANS2D/")
        HFIRSANS()
        FindBeamCenter("BioSANS_empty_cell.xml", FindBeamCenter.BEAMFINDER_DIRECT_BEAM)
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Reduce1D()
        
        data = mtd["Iq"].dataY(0)
        check = [0.268942,0.272052,0.269806,0.27129,0.273852,0.271301,0.271732,0.271103,0.270996,0.269677,0.27098,0.266802,0.26789,0.268222,0.266125,0.262736,0.262752,0.263827,0.26315,0.262775,0.261541,0.260818,0.258955,0.257675,0.255908,0.254088,0.256778,0.256883,0.253568,0.25636,0.252323,0.251833,0.251914,0.252298,0.249375,0.247718,0.247768,0.244636,0.245604,0.243996,0.244332,0.244363,0.242985,0.242234,0.241118,0.241411,0.24084,0.239293,0.2392,0.236565,0.234557,0.233974,0.232905,0.231898,0.231085,0.229586,0.22862,0.227001,0.226783,0.225837,0.224835,0.223807,0.222296,0.221557,0.220464,0.219139,0.217611,0.217049,0.21606,0.215739,0.216233,0.213467,0.213141,0.213275,0.219695,0.216121,0.215502,0.21792,0.209364,0.209368,0.2064,0.205844,0.20431,0.203443,0.202442,0.200195,0.199408,0.19853,0.195654,0.195514,0.193086,0.193388,0.19137,0.190122,0.189119,0.18864,0.185473,0.184958,0.183981,0.182581]

        for i in range(len(check)):
            self.assertAlmostEqual(data[i], check[i], 0.00001)

if __name__ == '__main__':
    unittest.main()
    