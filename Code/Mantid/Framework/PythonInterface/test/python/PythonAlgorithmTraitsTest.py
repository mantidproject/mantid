"""Defines tests for the traits within Python algorithms
such as name, version etc.
"""

import unittest
import testhelpers
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
    

###############################################################################

class PythonAlgorithmTest(unittest.TestCase):
        
    _registered = None
    
    def setUp(self):
        if self.__class__._registered is None:
            self.__class__._registered = True
            registerAlgorithm(TestPyAlgDefaultAttrs)
            registerAlgorithm(TestPyAlgOverriddenAttrs)
            
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
        testhelpers.assert_raises_nothing(self,AlgorithmManager.Instance().createUnmanaged, "TestPyAlgDefaultAttrs")
        alg = AlgorithmManager.Instance().createUnmanaged("TestPyAlgDefaultAttrs")
        testhelpers.assert_raises_nothing(self,alg.initialize)
       
        self.assertEquals(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "PythonAlgorithms")

    def test_alg_with_overridden_attrs(self):
        testhelpers.assert_raises_nothing(self,AlgorithmManager.Instance().createUnmanaged, "CoolAlgorithm")
        alg = AlgorithmManager.Instance().createUnmanaged("CoolAlgorithm")
        self.assertEquals(alg.name(), "CoolAlgorithm")
        self.assertEquals(alg.version(), 2)
        self.assertEquals(alg.category(), "BestAlgorithms")

if __name__ == '__main__':
    unittest.main()
