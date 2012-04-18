"""
    Specifically tests the Fit function in the simple API
"""
import unittest
import testhelpers

from mantid.simpleapi import Load, Fit, FitDialog
from mantid import mtd, MatrixWorkspace, ITableWorkspace

class SimpleAPIFitTest(unittest.TestCase):
    
    _raw_ws = None
    
    def setUp(self):
        if self._raw_ws is None:
            ws = Load('IRS21360.raw',SpectrumMax=1)
            self.__class__._raw_ws = ws

    def test_minimal_positional_arguments_work(self):
        testhelpers.assertRaisesNothing(self, Fit, "name=FlatBackground", self._raw_ws)

    def test_function_positional_and_workspace_keyword_arguments_work(self):
        testhelpers.assertRaisesNothing(self, Fit, "name=FlatBackground", InputWorkspace=self._raw_ws)

    def test_function_and_workspace_keyword_arguments_work(self):
        testhelpers.assertRaisesNothing(self, Fit, Function="name=FlatBackground", InputWorkspace=self._raw_ws)

    def test_function_returns_are_correct_type_when_no_output_ws_requested(self):
        retvals = Fit("name=FlatBackground", self._raw_ws)
        self.assertEquals(len(retvals), 2)
        self.assertTrue(isinstance(retvals[0], str))
        self.assertTrue(isinstance(retvals[1], float))

    def test_function_returns_are_correct_type_when_output_ws_is_requested(self):
        retvals = Fit("name=FlatBackground", self._raw_ws, Output="fitWS")
        self.assertEquals(len(retvals), 5)
        self.assertTrue(isinstance(retvals[0], str))
        self.assertTrue(isinstance(retvals[1], float))
        self.assertTrue(isinstance(retvals[2], ITableWorkspace))
        self.assertTrue(isinstance(retvals[3], ITableWorkspace))
        self.assertTrue(isinstance(retvals[4], MatrixWorkspace))
        
    def test_other_arguments_are_accepted_by_keyword(self):
        retvals = Fit("name=FlatBackground", self._raw_ws, MaxIterations=10, Output="fitWS")
        self.assertEquals(len(retvals), 5)
        self.assertTrue(isinstance(retvals[0], str))
        self.assertTrue(isinstance(retvals[1], float))
        self.assertTrue(isinstance(retvals[2], ITableWorkspace))
        self.assertTrue(isinstance(retvals[3], ITableWorkspace))
        self.assertTrue(isinstance(retvals[4], MatrixWorkspace))
        
    def test_that_dialog_call_raises_runtime_error(self):
        try:
            FitDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong: " + msg)


if __name__ == '__main__':
    unittest.main()