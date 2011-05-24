"""
    Unit tests for SANS reduction command set
"""
import unittest
import os
#from reduction.instruments.sans.sans_reducer import SANSReducer
#from reduction.instruments.sans.hfir_command_interface import *
from reduction.instruments.sans.eqsans_config import EQSANSConfig
from reduction.instruments.sans.sns_instrument import EQSANS
from mantidsimple import *



class TestCommands(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        data_path = mtd.getConfigProperty('datasearch.directories')
        self.config_file = None
        for p in data_path.split(';'):
            f = os.path.join(p.strip(),'eqsans_configuration.1463')
            if os.path.isfile(f):
                self.config_file = f
                
                
        if self.config_file is None:
            raise RuntimeError, "Could not find EQSANS config file"
        
    def test_read_config(self):
        conf = EQSANSConfig(self.config_file)
        self.assertEqual(len(conf.rectangular_masks), 16)
        self.assertEqual(conf.low_TOF_cut, 500)
        self.assertEqual(conf.high_TOF_cut, 500)


if __name__ == '__main__':
    unittest.main()
    