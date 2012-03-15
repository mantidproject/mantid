import unittest
import testhelpers
from mantid.api import FrameworkManager, FrameworkManagerImpl, AlgorithmProxy

class FrameworkManagerTest(unittest.TestCase):

    def test_clear_functions_do_not_throw(self):
        # Test they don't throw for now
        testhelpers.assertRaisesNothing(self, FrameworkManager.clear)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearData)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearAlgorithms)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearInstruments)
        
    def _is_managed_test(self, alg, version):
        self.assertTrue(alg.isInitialized())
        self.assertTrue(alg.version(), version)
        self.assertTrue(isinstance(alg, AlgorithmProxy))
        self.assertTrue(hasattr(alg, '__async__'))
        self.assertTrue(alg.__async__)
        
    def test_create_algorithm_produces_managed_alg_outside_PyExec_with_async_attr_and_is_true(self):
        alg = FrameworkManager.createAlgorithm("Rebin")
        self._is_managed_test(alg, 1)
        
    def test_create_algorithm_with_version_produces_managed_alg_outside_PyExec_with_async_and_is_true(self):
        alg = FrameworkManager.createAlgorithm("LoadRaw", 2)
        self._is_managed_test(alg, 2)
        
    def test_create_algorithm_produces_unmanaged_inside_PyExec_with_async_and_is_false(self):
        # A small test class to have a PyExec method call the 
        # algorithm creation
        class TestAlg(object):
            def __init__(self, test_object):
                self._test_obj = test_object
            
            def PyExec(self):
                alg = FrameworkManager.createAlgorithm("Rebin")
                self._test_obj.assertTrue(alg.isInitialized())
                self._test_obj.assertFalse(isinstance(alg, AlgorithmProxy))
                self._test_obj.assertTrue(hasattr(alg, '__async__'))
                self._test_obj.assertFalse(alg.__async__)
        
        top_level = TestAlg(self)
        top_level.PyExec()
        
if __name__ == '__main__':
    unittest.main()
