import unittest

from mantid.api import Algorithm, algorithm_mgr, register_algorithm

class TestPyAlgDefaultAttrs(Algorithm):
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass

class TestPyAlgOverriddenAttrs(Algorithm):
    
    def name(self):
        return 'CoolAlgorithm'
    
    def version(self):
        return 2
    
    def category(self):
        return "BestAlgorithms"
    
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass


class PythonAlgorithmTest(unittest.TestCase):
        
    _registered = None
    
    def setUp(self):
        if self.__class__._registered is None:
            self.__class__._registered = True
            register_algorithm(TestPyAlgDefaultAttrs)
            register_algorithm(TestPyAlgOverriddenAttrs)
        
    def raisesNothing(self, callable, *args): # unittest does not have this for some reason
        try:
            callable(*args)
        except RuntimeError, exc:
            self.fail(str(exc))
        
    def test_alg_with_default_attrs(self):
        self.raisesNothing(algorithm_mgr.create_unmanaged, "TestPyAlgDefaultAttrs")
        alg = algorithm_mgr.create_unmanaged("TestPyAlgDefaultAttrs")
        self.raisesNothing(alg.initialize)
       
        self.assertEquals(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "PythonAlgorithms")

    def test_alg_with_overridden_attrs(self):
        self.raisesNothing(algorithm_mgr.create_unmanaged, "CoolAlgorithm")
        alg = algorithm_mgr.create_unmanaged("CoolAlgorithm")
        self.assertEquals(alg.name(), "CoolAlgorithm")
        self.assertEquals(alg.version(), 2)
        self.assertEquals(alg.category(), "BestAlgorithms")

if __name__ == '__main__':
    unittest.main()