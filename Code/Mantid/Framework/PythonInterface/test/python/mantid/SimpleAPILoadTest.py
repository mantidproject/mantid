"""
    Specifically tests the Load function in the simple API
"""
import unittest
from mantid.simpleapi import Load, LoadDialog
from mantid.api import mtd, MatrixWorkspace, WorkspaceGroup

class SimpleAPILoadTest(unittest.TestCase):

    def tearDown(self):
        """Clear up after each test"""
        mtd.clear()

    def test_Load_uses_lhs_var_as_workspace_name_for_single_item_return(self):
        data = Load('IRS21360.raw')
        self._do_name_check(data, 'data')

    def test_Load_returns_correct_args_when_extra_output_props_are_added_at_execute_time(self):
        try:
            data, monitors = Load('IRS21360.raw', LoadMonitors='Separate')
        except Exception, exc:
            self.fail("An error occurred when returning outputs declared at algorithm execution: '%s'" % str(exc))

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self._do_name_check(data, 'data')

        self.assertTrue(isinstance(monitors, MatrixWorkspace))
        self._do_name_check(monitors, 'data_monitors')

    def test_Load_returns_just_the_WorkspaceGroup_when_final_output_is_a_group(self):
        data = Load('CSP78173.raw')
        self.assertTrue(isinstance(data, WorkspaceGroup))

    def test_Load_returns_only_the_WorkspaceGroups_when_final_output_is_a_group_and_monitors_are_separated(self):
        outputs = Load('CSP78173.raw', LoadMonitors='Separate')
        self.assertTrue(isinstance(outputs, tuple))
        self.assertEquals(len(outputs), 2)

        self.assertTrue(isinstance(outputs[0], WorkspaceGroup))
        self._do_name_check(outputs[0], 'outputs')
        self.assertTrue(isinstance(outputs[1], WorkspaceGroup))
        self._do_name_check(outputs[1], 'outputs_monitors')

    def test_Load_call_with_just_filename_executes_correctly(self):
        try:
            raw = Load('IRS21360.raw')
        except RuntimeError:
            self.fail("Load with a filename should not raise an exception")
        self.assertEquals(116, raw.getNumberHistograms())

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

    def test_Load_uses_OutputWorkspace_keyword_over_lhs_var_name_if_provided(self):
        wsname = 'test_Load_uses_OutputWorkspace_keyword_over_lhs_var_name_if_provided'
        wkspace = Load('IRS21360.raw',OutputWorkspace=wsname)
        self.assertTrue( wsname in mtd )

    def test_Load_accepts_EnableLogging_keyword(self):
        # The test here is that the algorithm runs without falling over about the EnableLogging keyword being a property
        wsname = 'test_Load_accepts_EnableLogging_keyword'
        Load('IRS21360.raw',OutputWorkspace=wsname, EnableLogging=False)
        self.assertTrue( wsname in mtd )

    def test_that_dialog_call_raises_runtime_error(self):
        try:
            LoadDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong")

    def _do_name_check(self, wkspace, expected_name):
        self.assertEqual(wkspace.getName(), expected_name)
        self.assertTrue(expected_name in mtd)

if __name__ == '__main__':
    unittest.main()
