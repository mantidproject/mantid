"""Tests for the migration with simple api function call replacements
"""
import unittest
import os

from MigrationTest import MigrationTest

__INPUTSTRING__ = r"""
LoadRaw("test-file.raw", 'testWS', SpectrumMax=1)
"""

__EXPECTEDSTRING__ = r"""
testWS = LoadRaw("test-file.raw")
"""

class SimpleAPIFunctionCallReplaceMigrationTest(MigrationTest):
    
    _test_filename = None
    _test_backupname = None
    _backup_ext = '.mantidbackup'

    def tearDown(self):
        """Clean up after a test"""
        self.remove_test_files()
        
    def test_function_returning_no_args_is_replaced_correctly(self):
        self.do_migration(__INPUTSTRING__)
        self.check_outcome(__INPUTSTRING__, __EXPECTEDSTRING__)
        
            
if __name__ == "__main__":
    unittest.main()