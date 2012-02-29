import unittest

from mantid import (PythonAlgorithm, AlgorithmProxy, Algorithm, IAlgorithm, 
                    AlgorithmManager, registerAlgorithm, Direction)

########################### Test classes #####################################

class TestPyAlgDefaultAttrs(PythonAlgorithm):
    def PyInit(self):
        pass
    
    def PyExec(self):
        pass

class TestPyAlgOverriddenAttrs(PythonAlgorithm):
    
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
    
class TestPyAlgDeclaringProps(PythonAlgorithm):
    
    def PyInit(self):
        self.declareProperty('Simplest', 1, Direction.Input) # Input property
        
    def PyExec(self):
        pass

###############################################################################

class PythonAlgorithmTest(unittest.TestCase):
        
    _registered = None
    
    def setUp(self):
        if self.__class__._registered is None:
            self.__class__._registered = True
            registerAlgorithm(TestPyAlgDefaultAttrs)
            registerAlgorithm(TestPyAlgOverriddenAttrs)
            registerAlgorithm(TestPyAlgDeclaringProps)
        
    def raisesNothing(self, callable, *args): # unittest does not have this for some reason
        try:
            callable(*args)
        except RuntimeError, exc:
            self.fail(str(exc))
        
    def test_managed_alg_is_descendent_of_AlgorithmProxy(self):
        alg = AlgorithmManager.Instance().create("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, AlgorithmProxy))
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_unmanaged_alg_is_descendent_of_PythonAlgorithm(self):
        alg = AlgorithmManager.Instance().createUnmanaged("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, PythonAlgorithm))
        self.assertTrue(isinstance(alg, Algorithm))
        self.assertTrue(isinstance(alg, IAlgorithm))
        
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
        
    def test_property_declarations(self):
        alg = AlgorithmManager.Instance().createUnmanaged("TestPyAlgDeclaringProps")
        props = alg.getProperties()
        self.assertEquals(0, len(props))
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(1, len(props))

if __name__ == '__main__':
    unittest.main()
