"""
    Unit tests for SANS reduction command set
"""
from __future__ import with_statement
import math
import sys
import os
import unittest
from sans_reducer import SANSReducer
from hfir_command_interface import *
from mantidsimple import *

# Set directory containg the test data, relative to the Mantid release directory.
#TEST_DIR = "../../../Test/Data/SANS2D/"
TEST_DIR = "/mnt/hgfs/workspace/mantid_trunk/Test/Data/SANS2D/"

def _diff_iq(x,y): return x-y
def _add(x,y): return x+y

def _read_IGOR(filepath):
    """
        Read in an HFIR IGOR output file with reduced data
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Skip first header line
        f.readline()
        for line in f:
            toks = line.split()
            try:
                q    = float(toks[0])
                iq   = float(toks[1])
                diq  = float(toks[2])
                data.append([q, iq, diq])
            except:
                print "_read_IGOR:", sys.exc_value  
    return data
     
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
    if not len(data_mantid)==len(data_igor):
        print "Incompatible data lengths"
        return False
    
    # Utility methods for manipulating the lists
    def _diff_chi2(x,y): return (x[1]-y[1])*(x[1]-y[1])/(x[2]*x[2])
    def _diff_iq(x,y): return x[1]-y[1]
    def _diff_err(x,y): return x[2]-y[2]
    def _add(x,y): return x+y
    
    # Check that I(q) is the same for both data sets
    deltas = map(_diff_iq, data_mantid, data_igor)
    delta  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta)>tolerance or math.isnan(delta):
        passed = False
        print "Sum of I(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta), tolerance)
    
    # Then compare the errors
    deltas = map(_diff_err, data_mantid, data_igor)
    delta_err  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta_err)>tolerance or math.isnan(delta):
        passed = False
        print "Sum of dI(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta_err), tolerance)
    
    # Compute chi2 of our result relative to IGOR 
    deltas = map(_diff_chi2, data_mantid, data_igor)
    chi2  = reduce(_add, deltas)/len(data_igor)
    if chi2>10.0*tolerance or math.isnan(delta):
        passed= False
        print "Chi2 is outside tolerance: %g > %g" % (chi2, 10.0*tolerance)
 
    return passed

class TestCommands(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        # The reducer is a singleton, so create a new instance for each unit test
        ReductionSingleton.clean(SANSReducer)
        self.assertEqual(ReductionSingleton()._sensitivity_correcter, None)
        self.assertEqual(ReductionSingleton()._transmission_calculator, None)
        self.assertEqual(ReductionSingleton()._save_iq, None)
        self.assertEqual(ReductionSingleton()._azimuthal_averager, None)
        self.assertEqual(ReductionSingleton()._beam_finder.__class__.__name__, "BaseBeamFinder")
        self.assertEqual(ReductionSingleton()._normalizer.__class__.__name__, "Normalize")
        self.assertEqual(ReductionSingleton()._solid_angle_correcter, None)
        self.assertEqual(len(ReductionSingleton()._reduction_steps), 0)
        self.assertEqual(len(ReductionSingleton()._data_files), 0)
        self.assertEqual(ReductionSingleton().instrument, None)
        self.assertEqual(ReductionSingleton()._data_path, '.')
        self.assertEqual(ReductionSingleton()._background_subtracter, None)
                
    def test_data_path(self):
        self.assertEqual(ReductionSingleton()._data_path, '.')
        #any path that definitely exists on a computer with Mantid installed
        test_path = mtd.getConfigProperty('instrumentDefinition.directory')
        DataPath(test_path)
        self.assertEqual(ReductionSingleton()._data_path, test_path)
        
    def test_set_detector_distance(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        AppendDataFile("BioSANS_test_data.xml")
        SetSampleDetectorDistance(2500.0)
        Reduce1D()
        
        sdd = mtd["BioSANS_test_data"].getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 2500.0)
        
    def test_set_detector_offset(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        AppendDataFile("BioSANS_test_data.xml")
        SetSampleDetectorOffset(500.0)
        Reduce1D()
        
        sdd = mtd["BioSANS_test_data"].getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 6500.0)
        
    def test_set_distandce_and_detector_offset(self):
        """
            If both detector distance and offset are set, use only the distance
        """
        HFIRSANS()
        DataPath(TEST_DIR)
        AppendDataFile("BioSANS_test_data.xml")
        SetSampleDetectorDistance(2500.0)
        SetSampleDetectorOffset(500.0)
        Reduce1D()
        
        sdd = mtd["BioSANS_test_data"].getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 2500.0)
        
    def test_set_wavelength(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        AppendDataFile("BioSANS_test_data.xml")
        SetWavelength(5.0, 1.2)
        Reduce1D()
        
        ws = mtd["BioSANS_test_data"]
        v_x = ws.dataX(0)
        self.assertEqual(v_x[0], 4.4)
        self.assertEqual(v_x[1], 5.6)
        
    def test_direct_beam_center(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
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
        HFIRSANS()
        DataPath(TEST_DIR)
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
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        self.assertEqual(ReductionSingleton()._normalizer._normalization_spectrum, 1)
        sdd = mtd["BioSANS_test_data"].getRun().getProperty("sample_detector_distance").value
        self.assertEqual(sdd, 6000.0)    
        
        self.assertTrue(_check_result(mtd["BioSANS_test_data_Iq"], TEST_DIR+"reduced_center_calculated.txt", tolerance=1e-4))
    
    def test_reduction_1(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        check = [0.19472,0.204269,0.215354,0.230114,0.238961,0.237201,0.247843,0.248424,0.253676,0.254327,0.254366,0.252931,0.258339,0.259297,0.257155,0.254059,0.252383,0.252826,0.256604,0.256754,0.255592,0.256813,0.248569,0.25331,0.251032,0.246424,0.249477,0.250939,0.251959,0.24925,0.250372,0.246148,0.250478,0.244621,0.247428,0.246431,0.245041,0.241647,0.24307,0.240096,0.242797,0.238182,0.237548,0.239789,0.241477,0.23456,0.237372,0.233715,0.233789,0.232262,0.231589,0.230986,0.231646,0.231331,0.230484,0.2277,0.226819,0.224341,0.227239,0.223228,0.221232,0.222011,0.224747,0.219533,0.216973,0.218734,0.21668,0.218366,0.214926,0.213985,0.214469,0.210473,0.209867,0.209066,0.208965,0.207498,0.204505,0.205786,0.202186,0.200442,0.200485,0.200554,0.200499,0.198152,0.193945,0.192082,0.193783,0.193787,0.190557,0.190471,0.186827,0.190088,0.188204,0.187547,0.182206,0.181384,0.180358,0.182663,0.178844,0.176556]

        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.00001)

    def test_no_solid_angle(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        NoSolidAngle()
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        self.assertAlmostEqual(data[0], 0.1948464330517794, 0.00001)
        self.assertAlmostEqual(data[10], 0.25088976280978281, 0.00001)
        self.assertAlmostEqual(data[20], 0.252098592791137, 0.00001)

    def test_reduction_2(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        check = [0.268942,0.272052,0.269806,0.27129,0.273852,0.271301,0.271732,0.271103,0.270996,0.269677,0.27098,0.266802,0.26789,0.268222,0.266125,0.262736,0.262752,0.263827,0.26315,0.262775,0.261541,0.260818,0.258955,0.257675,0.255908,0.254088,0.256778,0.256883,0.253568,0.25636,0.252323,0.251833,0.251914,0.252298,0.249375,0.247718,0.247768,0.244636,0.245604,0.243996,0.244332,0.244363,0.242985,0.242234,0.241118,0.241411,0.24084,0.239293,0.2392,0.236565,0.234557,0.233974,0.232905,0.231898,0.231085,0.229586,0.22862,0.227001,0.226783,0.225837,0.224835,0.223807,0.222296,0.221557,0.220464,0.219139,0.217611,0.217049,0.21606,0.215739,0.216233,0.213467,0.213141,0.213275,0.219695,0.216121,0.215502,0.21792,0.209364,0.209368,0.2064,0.205844,0.20431,0.203443,0.202442,0.200195,0.199408,0.19853,0.195654,0.195514,0.193086,0.193388,0.19137,0.190122,0.189119,0.18864,0.185473,0.184958,0.183981,0.182581]

        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.00001)

    def test_straight_Q1D(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        check = [0.269037,0.272176,0.269917,0.271416,0.273988,0.271432,0.271857,0.271232,0.271118,0.269797,0.271095,0.266912,0.268015,0.268356,0.266256,0.26287,0.262888,0.263964,0.263281,0.262905,0.261669,0.26094,0.259081,0.257802,0.256029,0.254228,0.256913,0.257021,0.253692,0.256491,0.252454,0.251969,0.25204,0.252423,0.249516,0.247844,0.247895,0.24476,0.245734,0.244125,0.244474,0.244491,0.243126,0.242359,0.241239,0.24154,0.240976,0.239421,0.23933,0.236688,0.234685,0.234105,0.233034,0.232036,0.231208,0.229714,0.228749,0.227122,0.226918,0.225969,0.22497,0.223933,0.222426,0.221684,0.2206,0.219277,0.217739,0.217173,0.216193,0.215869,0.216354,0.213597,0.213271,0.213407,0.219829,0.216259,0.215635,0.218058,0.209499,0.209503,0.206529,0.205981,0.20445,0.203577,0.202577,0.200334,0.199544,0.198663,0.195786,0.195653,0.19322,0.193537,0.191503,0.190253,0.189253,0.188771,0.1856,0.185099,0.184111,0.182717]

        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.00001)

    def test_transmission(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
    
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                               empty_file="BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        check = [0.514758,0.520759,0.516451,0.51932,0.524206,0.519275,0.520125,0.518997,0.518729,0.516198,0.518718,0.51072,0.512816,0.513449,0.509453,0.502968,0.503003,0.505098,0.503835,0.503088,0.500716,0.499304,0.495777,0.49332,0.489926,0.486497,0.491656,0.491858,0.48546,0.490808,0.483111,0.482176,0.482359,0.483098,0.477528,0.474279,0.474485,0.468472,0.470305,0.467228,0.467934,0.467971,0.465358,0.463885,0.461762,0.462352,0.461285,0.458322,0.458118,0.453064,0.44927,0.448151,0.446129,0.444207,0.442629,0.439792,0.437958,0.434826,0.434443,0.432655,0.430731,0.428771,0.425893,0.424477,0.422421,0.419886,0.416942,0.415876,0.414037,0.41339,0.414353,0.409062,0.408431,0.408712,0.419282,0.412833,0.41062,0.414427,0.400056,0.400141,0.394724,0.393821,0.390721,0.38932,0.387497,0.383062,0.381603,0.380016,0.374635,0.374214,0.369733,0.370353,0.366464,0.364109,0.362184,0.361299,0.355246,0.354339,0.352412,0.349748]

        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.00001)
                
    def test_spreader_transmission(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        BeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml", 
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml", 
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5, 
                                 spreader_transmission_err=0.1)
        
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        self.assertAlmostEqual(data[0], 0.00418831, 0.00001)
        self.assertAlmostEqual(data[10], 0.0042193, 0.00001)

    def test_transmission_by_hand(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        trans = ReductionSingleton()._transmission_calculator.get_transmission()
        self.assertEqual(trans[0],0.51944)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
        self.assertTrue(_check_result(mtd["BioSANS_test_data_Iq"], TEST_DIR+"reduced_transmission.txt", 0.0001))
            
    def test_center_by_hand(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
        self.assertTrue(_check_result(mtd["BioSANS_test_data_Iq"], TEST_DIR+"reduced_center_by_hand.txt", 0.0001))
            
    def test_background(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        self.assertEqual(data[0], 0.0)
        self.assertEqual(data[10], 0.0)
        self.assertEqual(data[20], 0.0)
            
    def test_bck_w_transmission(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml", "test_data")
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        DarkCurrent("BioSANS_dark_current.xml")
        Background("BioSANS_test_data.xml")
        SetTransmission(0.6,0.1)
        SetBckTransmission(0.6,0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
        data = mtd["test_data_Iq"].dataY(0)
        self.assertEqual(data[0], 0.0)
        self.assertEqual(data[10], 0.0)
        self.assertEqual(data[20], 0.0)
            
    def test_transmission_by_hand_w_sensitivity(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml")
        trans = ReductionSingleton()._transmission_calculator.get_transmission()
        self.assertEqual(trans[0],0.51944)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
        
        #self.assertTrue(_check_result(mtd["BioSANS_test_data_Iq"], TEST_DIR+"reduced_transmission.txt"))
        
        data = mtd["BioSANS_test_data_Iq"].dataY(0)
        check = [0.374914,0.393394,0.414756,0.443152,0.460175,0.456802,0.477264,0.478456,0.488523,0.489758,0.489871,0.487127,0.497585,0.499346,0.49526,0.489273,0.486082,0.486923,0.494208,0.494531,0.492264,0.494608,0.478766,0.487872,0.48357,0.474654,0.48052,0.483367,0.485269,0.480079,0.482254,0.47413,0.48245,0.471207,0.476589,0.474701,0.472014,0.465479,0.468236,0.462524,0.46773,0.458851,0.457653,0.461929,0.465216,0.451887,0.45733,0.450281,0.45045,0.447508,0.446209,0.445063,0.446328,0.445735,0.444096,0.438758,0.43707,0.432302,0.437903,0.430176,0.426317,0.427858,0.433131,0.423087,0.418146,0.421584,0.417606,0.420891,0.414255,0.412448,0.413393,0.405706,0.404541,0.403016,0.402806,0.400023,0.394248,0.396725,0.389808,0.386475,0.386525,0.386674,0.386575,0.382081,0.373986,0.370391,0.37367,0.373686,0.367479,0.36732,0.36031,0.366588,0.362994,0.361712,0.351433,0.349867,0.3479,0.352355,0.344987,0.340605]

        # Check that I(q) is the same for both data sets
        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.00001)
            
    def test_SampleGeometry_functions(self):
        HFIRSANS()
        DataPath(TEST_DIR)
        AppendDataFile("BioSANS_test_data.xml")   
        SampleGeometry('cuboid')
        SampleThickness(2.0)
        SampleHeight(3.0)
        SampleWidth(5.0)
        
        # we don't need to do a full reduction for this test, do a partial reduction
        ReductionSingleton().pre_process()
        ReductionSingleton()._reduction_steps[0].execute(ReductionSingleton(), "BioSANS_test_data")
        ReductionSingleton().geometry_correcter.execute(ReductionSingleton(), "BioSANS_test_data")
          
        ws = mtd["BioSANS_test_data"]
        data = [ws.dataY(0)[0], ws.dataY(1)[0], ws.dataY(2)[0], ws.dataY(3)[0], ws.dataY(4)[0], ws.dataY(5)[0]]
        
        check = [500091.0,60.0,40.8333,13.6333, 13.4667,13.6667]
                # Check that I(q) is the same for both data sets
        deltas = map(_diff_iq, data, check)
        delta  = reduce(_add, deltas)/len(deltas)
        self.assertTrue(math.fabs(delta)<0.1)
            

if __name__ == '__main__':
    unittest.main()
    