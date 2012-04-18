"""
    Specifically tests the Fit function in the simple API
"""
import unittest
import sys
from mantidsimple import Load, Fit, FitDialog, mtd

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

class SimpleAPIFitTest(unittest.TestCase):
    
    _raw_ws = None
    
    def setUp(self):
        if self._raw_ws is None:
            Load('IRS21360.raw',OutputWorkspace='ws', SpectrumMax=1)
            self.__class__._raw_ws = mtd['ws']

    def test_minimal_positional_arguments_work(self):
        assertRaisesNothing(self, Fit, "name=FlatBackground", self._raw_ws)

    def test_function_positional_and_workspace_keyword_arguments_work(self):
        assertRaisesNothing(self, Fit, "name=FlatBackground", InputWorkspace=self._raw_ws)

    def test_function_and_workspace_keyword_arguments_work(self):
        assertRaisesNothing(self, Fit, Function="name=FlatBackground", InputWorkspace=self._raw_ws)

    def xtest_workspace_is_created_when_output_is_requested(self): # crashes the mac
        retvals = Fit("name=FlatBackground", self._raw_ws, Output="fitWS")
        sys.__stderr__.write("")
        self.assertTrue('fitWS_Workspace' in mtd)
        
    def xtest_other_arguments_are_accepted_by_keyword(self): # crashes the mac
        retvals = Fit("name=FlatBackground", self._raw_ws, MaxIterations=10, Output="fitWS")
        self.assertTrue('fitWS_Workspace' in mtd)
        
    def test_that_dialog_call_raises_runtime_error(self):
        try:
            FitDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong: " + msg)

if __name__ == '__main__':
    unittest.main()