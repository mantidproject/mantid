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
