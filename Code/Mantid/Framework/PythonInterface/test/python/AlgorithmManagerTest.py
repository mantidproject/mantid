import unittest
import testhelpers
from mantid.api import AlgorithmManager
from mantid.api import (IAlgorithm, Algorithm, AlgorithmProxy, PythonAlgorithm, 
                        registerAlgorithm)
import sys

class IsAnAlgorithm(PythonAlgorithm):
    def PyInit(self):
        pass
    
class NotAnAlgorithm(object):
    pass

class AlgorithmManagerTest(unittest.TestCase):
    
    def test_create_default_version(self):
        alg = testhelpers.assert_raises_nothing(self, AlgorithmManager.Instance().create, "ConvertUnits")
        # Tests
        self.assertNotEqual(alg, None)
        self.assertEquals(alg.name(), "ConvertUnits")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "Transforms\\Units")
        
    def test_create_unknown_alg_throws(self):
        self.assertRaises(RuntimeError, AlgorithmManager.Instance().create,"DoesNotExist")
        
    def test_created_alg_isinstance_of_IAlgorithm(self):
        alg = AlgorithmManager.Instance().create("ConvertUnits")
        self.assertTrue(isinstance(alg, IAlgorithm))
        
    def test_managed_cppalg_isinstance_of_AlgorithmProxy(self):
        alg = AlgorithmManager.Instance().create("ConvertUnits")
        self.assertTrue(isinstance(alg, AlgorithmProxy))

    def test_unmanaged_cppalg_isinstance_of_Algorithm(self):
        alg = AlgorithmManager.Instance().createUnmanaged("ConvertUnits")
        self.assertTrue(isinstance(alg, Algorithm))
        
    def test_pyalg_isinstance_of_Algorithm(self):
        alg = IsAnAlgorithm()
        self.assertTrue(isinstance(alg, Algorithm))
        self.assertTrue(isinstance(alg, IAlgorithm))
        
    def test_algorithm_registration_with_valid_object_succeeds(self):
        testhelpers.assert_raises_nothing(self, registerAlgorithm, IsAnAlgorithm)

    def test_algorithm_registration_with_invalid_object_throws(self):
        self.assertRaises(ValueError, registerAlgorithm, NotAnAlgorithm)

if __name__ == '__main__':
    unittest.main()        
