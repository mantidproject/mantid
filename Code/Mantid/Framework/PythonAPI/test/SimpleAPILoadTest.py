"""
    Specifically tests the Load function in the simple API
"""
import unittest
from mantidsimple import Load, LoadDialog, mtd

# Case difference is to be consistent with the unittest module
def assertRaisesNothing(testobj, callable, *args, **kwargs): 
    """
        unittest does not have an assertRaisesNothing. This
        provides that functionality
    
        Parameters:
            testobj  - A unittest object
            callable - A callable object
            *args    - Positional arguments passed to the callable as they are
            **kwargs - Keyword arguments, passed on as they are
    """
    try:
         return callable(*args, **kwargs)
    except Exception, exc:
        testobj.fail("Assertion error. An exception was caught where none was expected in %s. Message: %s" 
                     % (callable.__name__, str(exc)))

class SimpleAPILoadTest(unittest.TestCase):
    
    def test_Load_call_with_just_filename_and_workspace_executes_correctly(self):
        assertRaisesNothing(self, Load, 'IRS21360.raw', 'raw')
        self._run_check_and_remove('raw', 116)

    def test_Load_call_with_other_args_executes_correctly(self):
        assertRaisesNothing(self, Load, 'IRS21360.raw', 'raw', SpectrumMax=1)
        self._run_check_and_remove('raw', 1)

    def test_Load_call_with_all_keyword_args_executes_correctly(self):
        assertRaisesNothing(self, Load, Filename='IRS21360.raw', OutputWorkspace='raw', SpectrumMax=1)
        self._run_check_and_remove('raw', 1)
        
    def test_Load_call_with_args_that_do_not_apply_executes_correctly(self):
        assertRaisesNothing(self, Load, 'IRS21360.raw', 'raw', SpectrumMax=1,Append=True)
        self._run_check_and_remove('raw', 1)

    def test_that_dialog_call_raises_runtime_error(self):
        try:
            LoadDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong: " + msg)

    def _run_check_and_remove(self, name, expected_nhist):
        raw = mtd[name]
        self.assertEquals(expected_nhist, raw.getNumberHistograms())
        mtd.deleteWorkspace(name)




if __name__ == '__main__':
    unittest.main()
