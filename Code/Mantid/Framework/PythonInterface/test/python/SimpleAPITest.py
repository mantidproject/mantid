import unittest
import sys

from mantid.api import algorithm_factory

from mantid import simpleapi

class SimpleAPITest(unittest.TestCase):
    
    def test_module_dict_seems_to_be_correct_size(self):
        # Check that the module has at least the same number
        # of attributes as unique algorithms
        module_dict = dir(simpleapi)
        all_algs = algorithm_factory.get_registered_algorithms(True)
        self.assertTrue( len(module_dict) > len(all_algs) )