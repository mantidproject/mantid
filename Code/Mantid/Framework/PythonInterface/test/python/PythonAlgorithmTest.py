import unittest

from mantid.api import PythonAlgorithm, algorithm_mgr, register_algorithm

class TestPyAlgDefaultAttrs(PythonAlgorithm):
    def init_(self):
        pass
    
    def exec_(self):
        pass

class TestPyAlgOverriddenAttrs(PythonAlgorithm):
    
    def name_(self):
        return 'CoolAlgorithm'
    
    def version_(self):
        return 2
    
    def category_(self):
        return "BestAlgorithms"
    
    def init_(self):
        pass
    
    def exec_(self):
        pass

register_algorithm(TestPyAlgDefaultAttrs)
register_algorithm(TestPyAlgOverriddenAttrs)

class PythonAlgorithmTest(unittest.TestCase):
        
    def raisesNothing(self, callable, *args): # unittest does not have this for some reason
        try:
            callable(*args)
        except RuntimeError, exc:
            self.fail(str(exc))
        
    def test_alg_with_default_attrs(self):
        self.raisesNothing(algorithm_mgr.create_unmanaged, "TestPyAlgDefaultAttrs")
        alg = algorithm_mgr.create_unmanaged("TestPyAlgDefaultAttrs")
        self.assertEquals(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "PythonAlgorithms")

    def test_alg_with_overridden_attrs(self):
        self.raisesNothing(algorithm_mgr.create_unmanaged, "CoolAlgorithm")
        alg = algorithm_mgr.create_unmanaged("CoolAlgorithm")
        self.assertEquals(alg.name(), "CoolAlgorithm")
        self.assertEquals(alg.version(), 2)
        self.assertEquals(alg.category(), "BestAlgorithms")

        