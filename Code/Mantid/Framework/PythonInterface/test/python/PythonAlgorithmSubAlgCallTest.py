"""A test for the simple API dedicated to Python algorithms. Checks
things like sub-algorithm calls
"""
import unittest
import inspect
import os
import shutil


#import mantid.simpleapi as simpleapi


class PythonAlgorithmSubAlgCallTest(unittest.TestCase):
    
    _testdir = os.path.join(os.getcwd(), 'PythonAlgorithmSimpleAPITest_TmpDir')
    _parentalg = None
    _childalg = None
    
    def test_subalg_call_of_one_python_call_from_other_suceeds(self):
         
    
    def setUp(self):
        __PARENTALG__ = \
        """from mantid import PythonAlgorithm, registerAlgorithm
        
        class ParentAlgorithm(PythonAlgorithm):
        
            def PyInit(self):
                pass
            
            def PyExec(self):
                ChildAlgorithm() # If not setup correct this will raise a RuntimeError
                
        registerAlgorithm(ParentAlgorithm)
        """
        
        __CHILDALG__ = \
        """from mantid import PythonAlgorithm, registerAlgorithm
        
        class ChildAlgorithm(PythonAlgorithm):
        
            def PyInit(self):
                pass
            
            def PyExec(self):
                pass
                
        registerAlgorithm(ChildAlgorithm)
        """
        try:
            os.mkdir(self._testdir)
        except OSError:
            pass # Already exists, maybe it was not removed when a test failed?
        _parentalg = os.path.join(self._testdir, 'ParentAlgorithm.py')
        if not os.path.exists(_parentalg):
            plugin = file(_parentalg, 'w')
            plugin.write(__PARENTALG__)
            plugin.close()
        _childalg = os.path.join(self._testdir, 'ChildAlgorithm.py')
        if not os.path.exists(_parentalg):
            plugin = file(_parentalg, 'w')
            plugin.write(__CHILDALG__)
            plugin.close()
        
        __import__
    

    def tearDown(self):
        try:
            shutil.rmtree(self._testdir)
        except shutil.Error:
            pass

if __name__ == '__main__':
    unittest.main()
