import unittest
import testhelpers
from mantid.api import FrameworkManager, FrameworkManagerImpl, IAlgorithm, AlgorithmProxy

def _is_initialized_test(testobj, alg, version, expected_class, expected_child):
    testobj.assertTrue(alg.isInitialized())
    testobj.assertEquals(expected_child,alg.isChild())
    testobj.assertEquals(alg.version(), version)
    testobj.assertTrue(isinstance(alg, expected_class))


class FrameworkManagerTest(unittest.TestCase):

    def test_clear_functions_do_not_throw(self):
        # Test they don't throw for now
        testhelpers.assertRaisesNothing(self, FrameworkManager.clear)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearData)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearAlgorithms)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearInstruments)
        
    def test_create_algorithm_produces_initialized_alorithm(self):
        alg = FrameworkManager.createAlgorithm("Rebin")
        _is_initialized_test(self, alg, 1, expected_class=AlgorithmProxy, 
                             expected_child=False)
        
    def test_create_algorithm_with_version_produces_initialized_alorithm(self):
        alg = FrameworkManager.createAlgorithm("LoadRaw", 2)
        _is_initialized_test(self, alg, 2, expected_class=AlgorithmProxy,
                             expected_child=False)
        
    def test_create_algorithm_produces_child_inside_PyExec(self):
        # A small test class to have a PyExec method call the 
        # algorithm creation
        class TestAlg(object):
            def __init__(self, test_object):
                self._test_obj = test_object
            
            def PyExec(self):
                alg = FrameworkManager.createAlgorithm("Rebin")
                _is_initialized_test(self._test_obj, alg, 1,
                                     expected_class=IAlgorithm, expected_child=True)
        
        top_level = TestAlg(self)
        top_level.PyExec()
        
if __name__ == '__main__':
    unittest.main()
