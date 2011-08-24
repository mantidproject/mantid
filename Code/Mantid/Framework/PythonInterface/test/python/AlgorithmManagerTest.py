import unittest

from mantid.api import algorithm_mgr

class AlgorithmManagerTest(unittest.TestCase):
  
    def test_create_default_version(self):
        try:
            alg = algorithm_mgr.create("ConvertUnits")
            noerror = True
        except RuntimeError:
            noerror = False
        # Tests
        self.assertEquals(noerror, True)
        self.assertNotEqual(alg, None)
        self.assertEquals(alg.name(), "ConvertUnits")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "Units")
        
    