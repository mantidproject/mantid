"""
    Specifically tests the Load function in the simple API
"""
import unittest
from mantid.simpleapi import Load, LoadDialog
from mantid import mtd

class SimpleAPILoadTest(unittest.TestCase):
    
    def test_Load_returns_correct_args_when_extra_output_props_are_added_at_execute_time(self):
        try:
            data, monitors = Load('IRS21360.raw', LoadMonitors='Separate')
        except Exception, exc:
            self.fail("An error occurred when returning outputs declared at algorithm execution: '%s'" % str(exc))
    
    def test_Load_call_with_just_filename_executes_correctly(self):
        try:
            raw = Load('IRS21360.raw')
        except RuntimeError:
            self.fail("Load with a filename should not raise an exception")
        self.assertEquals(116, raw.getNumberHistograms())
        mtd.remove('raw')

    def test_Load_call_with_other_args_executes_correctly(self):
        try:
            raw = Load('IRS21360.raw',SpectrumMax=1)
        except RuntimeError:
            self.fail("Load with a filename and extra args should not raise an exception")
        self.assertEquals(1, raw.getNumberHistograms())
        
    def test_Load_call_with_all_keyword_args_executes_correctly(self):
        raw = Load(Filename='IRS21360.raw', SpectrumMax=1)
        self.assertEquals(1, raw.getNumberHistograms())

    def test_Load_call_with_args_that_do_not_apply_executes_correctly(self):
        try:
            raw = Load('IRS21360.raw',SpectrumMax=1,Append=True)
        except RuntimeError:
            self.fail("Load with a filename and extra args should not raise an exception")
        self.assertEquals(1, raw.getNumberHistograms())

    def test_that_dialog_call_raises_runtime_error(self):
        try:
            LoadDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong")


if __name__ == '__main__':
    unittest.main()
