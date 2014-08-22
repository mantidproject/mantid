"""Tests for the migration with simple api function call replacements
"""
import unittest
import os

from MigrationTest import MigrationTest

class SimpleAPIFunctionCallReplaceMigrationTest(MigrationTest):

    _test_filename = None
    _test_backupname = None
    _backup_ext = '.mantidbackup'

    def tearDown(self):
        """Clean up after a test"""
        self.remove_test_files()

    def test_single_arg_no_keyword_is_migrated_correctly(self):
        inputstring = """DeleteWorkspace(ws)"""
        expected = """DeleteWorkspace(Workspace=ws)"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_single_arg_with_nested_call(self):
        inputstring = """DeleteWorkspace(kwargs[ws])"""
        expected = """DeleteWorkspace(Workspace=kwargs[ws])"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_single_arg_keyword_is_migrated_correctly(self):
        inputstring = """DeleteWorkspace(Workspace=ws)"""
        expected = """DeleteWorkspace(Workspace=ws)"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_all_keyword_is_untouched(self):
        inputstring = """LoadRaw(Filename='test.raw',OutputWorkspace=ws)"""
        expected = """LoadRaw(Filename='test.raw',OutputWorkspace=ws)"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_function_containing_list_arg_is_correctly_migrated(self):
        inputstring = """Rebin(ws,ws,[1,2,3])"""
        expected = """Rebin(InputWorkspace=ws,OutputWorkspace=ws,Params=[1,2,3])"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_no_arg_no_indent_is_migrated_correctly(self):
        inputstring = """LoadRaw("test-file.raw",'testWS',SpectrumMax=1)"""
        expected = """LoadRaw(Filename="test-file.raw",OutputWorkspace='testWS',SpectrumMax=1)"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_function_returning_no_args_is_replaced_correctly(self):
        inputstring = """
def foo():
    LoadRaw("test-file.raw",'testWS',SpectrumMax=1)
"""
        expected = """
def foo():
    LoadRaw(Filename="test-file.raw",OutputWorkspace='testWS',SpectrumMax=1)
"""
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_function_call_split_over_multiple_lines_is_replaced_correctly(self):
        inputstring = """
LoadRaw("test-file.raw",'testWS',
        SpectrumMax=1)"""
        expected = """
LoadRaw(Filename="test-file.raw",OutputWorkspace='testWS',
        SpectrumMax=1)"""
        self.create_test_file(inputstring)
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

if __name__ == "__main__":
    unittest.main()
