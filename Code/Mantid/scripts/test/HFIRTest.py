"""
    Unit tests for SANS reduction command set
"""
import unittest
from reduction.instruments.sans.sans_reducer import SANSReducer
from reduction.instruments.sans.hfir_command_interface import *
from mantidsimple import *



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
                
    def test_load_run(self):
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
        
            

if __name__ == '__main__':
    unittest.main()
    