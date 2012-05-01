"""Tests for the migration with Python algorithms
"""
import unittest
import os

from MigrationTest import MigrationTest

class PythonAlgorithmlReplaceMigrationTest(MigrationTest):
    
    def tearDown(self):
        """Clean up after a test"""
        self.remove_test_files()

    def xtest_no_property_alg_is_correct(self):
        inputstring = \
        """
        from MantidFramework import *
        mtd.initialize()
        
        class MyAlgorithm(PythonAlgorithm):
            
            def PyInit(self):
                pass
                
            def PyExec(self):
                pass
                
        mtd.registerPyAlgorithm(MyAlgorithm())
        """
        expected = \
        """
        from mantid import *
        
        from mantid.pythonalgorithms import *
        
        class MyAlgorithm(PythonAlgorithm):
            
            def PyInit(self):
                pass
                
            def PyExec(self):
                pass
                
        registerAlgorithm(MyAlgorithm)
        """
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

if __name__ == "__main__":
    unittest.main()
