import unittest

from mantid.api import framework_mgr, AlgorithmProxy

class FrameworkManagerTest(unittest.TestCase):

    def assertRaisesNothing(self, callable):
        try:
            callable()
        except Exception, exc:
            self.fail(str(exc))            

    def test_clear_functions_do_not_throw(self):
        # Test they don't throw for now
        self.assertRaisesNothing(framework_mgr.clear)
        self.assertRaisesNothing(framework_mgr.clear_data)
        self.assertRaisesNothing(framework_mgr.clear_algorithms)
        self.assertRaisesNothing(framework_mgr.clear_instruments)
        
    def _is_managed_test(self, alg, version):
        self.assertTrue(alg.is_initialized())
        self.assertTrue(alg.version(), version)
        self.assertTrue(isinstance(alg, AlgorithmProxy))
        self.assertTrue(hasattr(alg, '__async__'))
        self.assertTrue(alg.__async__)
        
    def test_create_algorithm_produces_managed_alg_outside_PyExec_with_async_attr_and_is_true(self):
        alg = framework_mgr.create_algorithm("Rebin")
        self._is_managed_test(alg, 1)
        
    def test_create_algorithm_with_version_produces_managed_alg_outside_PyExec_with_async_and_is_true(self):
        alg = framework_mgr.create_algorithm("LoadRaw", 2)
        self._is_managed_test(alg, 2)
        
    def test_create_algorithm_produces_unmanaged_inside_PyExec_with_async_and_is_false(self):
        # A small test class to have a PyExec method call the 
        # algorithm creation
        class TestAlg(object):
            def __init__(self, test_object):
                self._test_obj = test_object
            
            def PyExec(self):
                alg = framework_mgr.create_algorithm("Rebin")
                self._test_obj.assertTrue(alg.is_initialized())
                self._test_obj.assertFalse(isinstance(alg, AlgorithmProxy))
                self._test_obj.assertTrue(hasattr(alg, '__async__'))
                self._test_obj.assertFalse(alg.__async__)
        
        top_level = TestAlg(self)
        top_level.PyExec()