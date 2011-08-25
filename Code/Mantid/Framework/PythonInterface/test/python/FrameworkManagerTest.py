import unittest

from mantid.api import framework_mgr

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