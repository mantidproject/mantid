import unittest

from mantid.api import algorithm_mgr
from mantid.api import (IAlgorithm, Algorithm, AlgorithmProxy, 
                        PythonAlgorithm, register_algorithm)
import sys

class IsAnAlgorithm(PythonAlgorithm):
    def init_(self):
        pass
    
class NotAnAlgorithm(object):
    pass

class AlgorithmManagerTest(unittest.TestCase):
    
    def test_create_default_version(self):
        try:
            alg = algorithm_mgr.create("ConvertUnits")
        except RuntimeError:
            self.fail(str(exc))
        # Tests
        self.assertNotEqual(alg, None)
        self.assertEquals(alg.name(), "ConvertUnits")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "Units")
        
    def test_create_unknown_alg_throws(self):
        self.assertRaises(RuntimeError, algorithm_mgr.create,"DoesNotExist")
        
    def test_created_alg_isinstance_of_IAlgorithm(self):
        alg = algorithm_mgr.create("ConvertUnits")
        self.assertTrue(isinstance(alg, IAlgorithm))
        
    def test_managed_cppalg_isinstance_of_AlgorithmProxy(self):
        alg = algorithm_mgr.create("ConvertUnits")
        self.assertTrue(isinstance(alg, AlgorithmProxy))

    def test_unmanaged_cppalg_isinstance_of_Algorithm(self):
        alg = algorithm_mgr.create_unmanaged("ConvertUnits")
        self.assertTrue(isinstance(alg, Algorithm))
        
    def test_pyalg_isinstance_of_PythonAlgorithm(self):
        alg = IsAnAlgorithm()
        self.assertTrue(isinstance(alg, PythonAlgorithm))
        self.assertTrue(isinstance(alg, Algorithm))
        self.assertTrue(isinstance(alg, IAlgorithm))
        
    def test_algorithm_registration_with_valid_object_succeeds(self):
        try:
            register_algorithm(IsAnAlgorithm)
            noerror = True
        except Exception:
            noerror = False
        self.assertTrue(noerror)

    def test_algorithm_registration_with_invalid_object_throws(self):
        try:
            register_algorithm(NotAnAlgorithm)
            error = False
        except ValueError:
            error = True
        self.assertTrue(error)
        