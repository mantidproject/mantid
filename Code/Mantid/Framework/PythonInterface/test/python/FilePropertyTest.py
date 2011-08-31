import unittest

from testhelpers import run_algorithm

from mantid.api import algorithm_mgr, FileProperty

class FilePropertyTest(unittest.TestCase):
  
    def test_alg_get_property_converts_to_this(self):
        alg = run_algorithm('Load', Filename='ALF15739.raw', OutputWorkspace='tmp', SpectrumMax=1)
        prop = alg.get_property("Filename")
        self.assertEquals(type(prop), FileProperty)
        self.assertTrue('value' in dir(prop)) # Do we have a value method