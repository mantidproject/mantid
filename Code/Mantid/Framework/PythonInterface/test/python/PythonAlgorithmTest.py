import unittest

from mantid.api import Algorithm, AlgorithmManager, registerAlgorithm

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
            registerAlgorithm(TestPyAlgDefaultAttrs)
            registerAlgorithm(TestPyAlgOverriddenAttrs)
        
    def raisesNothing(self, callable, *args): # unittest does not have this for some reason
        try:
            callable(*args)
        except RuntimeError, exc:
            self.fail(str(exc))
        
    def test_alg_with_default_attrs(self):
        self.raisesNothing(AlgorithmManager.Instance().createUnmanaged, "TestPyAlgDefaultAttrs")
        alg = AlgorithmManager.Instance().createUnmanaged("TestPyAlgDefaultAttrs")
        self.raisesNothing(alg.initialize)
       
        self.assertEquals(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "PythonAlgorithms")

    def test_alg_with_overridden_attrs(self):
        self.raisesNothing(AlgorithmManager.Instance().createUnmanaged, "CoolAlgorithm")
        alg = AlgorithmManager.Instance().createUnmanaged("CoolAlgorithm")
        self.assertEquals(alg.name(), "CoolAlgorithm")
        self.assertEquals(alg.version(), 2)
        self.assertEquals(alg.category(), "BestAlgorithms")

if __name__ == '__main__':
    unittest.main()
