import unittest
import mantid
from mantid.simpleapi import LoadAscii, ConvertToHistogram, DeleteWorkspace, mtd
from reduction import Reducer, ReductionStep, validate_step

# Make sure we can import the UI
from reduction_application import ReductionGUI

class TestReductionStep(ReductionStep):
    def __init__(self):
        self.some_value = 5
    def execute(self, reducer, workspace):
        return self.some_value
    
class ReducerTest(unittest.TestCase):
    
    def test_append_step(self):
        """
            Test that a Mantid algorithm function can be added to a Reducer object
        """
        r = Reducer()
        # An algorithm with a mandatory property that is NOT InputWorkspace or OutputWorkspace
        r.append_step(LoadAscii, "AsciiExample.txt", None)
        # Algorithm with InputWorkspace and OutputWorkspace
        r.append_step(ConvertToHistogram, None, None)
        for item in r._reduction_steps:
            result = item.execute(r, "test2")
            
        # Check that the workspace was created
        self.assertTrue("test2" in mtd)
        DeleteWorkspace("test2")

    def test_pars_variation(self):
        """
            Variations for parameter specification
        """
        r = Reducer()
        # An algorithm with a mandatory property that is NOT InputWorkspace or OutputWorkspace
        r.append_step(LoadAscii, Filename="AsciiExample.txt", OutputWorkspace=None)
        # Algorithm with InputWorkspace and OutputWorkspace
        r.append_step(ConvertToHistogram, None, None)
        for item in r._reduction_steps:
            result = item.execute(r, "test2")
            
        # Check that the workspace was created
        self.assertTrue("test2" in mtd)
        DeleteWorkspace("test2")
                
    def test_output_wksp(self):
        """
            Similar to previous test, but we specify the output workspace
        """
        r = Reducer()
        # An algorithm with a mandatory property that is NOT InputWorkspace or OutputWorkspace
        r.append_step(LoadAscii, "AsciiExample.txt", None)
        # Algorithm with InputWorkspace and OutputWorkspace
        r.append_step(ConvertToHistogram, None, None)
        
        r._reduction_steps[0].execute(r, "test2")

        r._reduction_steps[1].execute(r, "test2", "test3")
            
        # Check that the workspace was created
        self.assertTrue("test2" in mtd)
        self.assertTrue("test3" in mtd)
        DeleteWorkspace("test2")
        DeleteWorkspace("test3")
        
    def test_parameter_variation(self):
        """
            Similar to previous test, but the algo function is passed as a string
        """
        r = Reducer()
        r.append_step("LoadAscii", "AsciiExample.txt", None)
        for item in r._reduction_steps:
            result = item.execute(r, "test2")
            
        # Check that the workspace was created
        self.assertTrue("test2" in mtd)
        DeleteWorkspace("test2")
            
    def test_reduction_step(self):
        """
            Test that passing a ReductionStep object works 
        """
        r = Reducer()
        r.append_step(TestReductionStep())
        for item in r._reduction_steps:
            result = item.execute(r, "test2")
        self.assertEqual(result, 5)
        
    def test_decorator(self):
        """
            Check that the decorator works for any method with a
            signature like func(reducer, algorithm)
        """
        @validate_step
        def some_func(reducer, algorithm):
            self.assertTrue(issubclass(type(algorithm), ReductionStep))
            
        some_func(Reducer(), LoadAscii, "AsciiExample.txt", None)
        
    def test_bad_alg_name(self):
        r = Reducer()
        r.append_step("NotAnAlgorithm")
        self.assertRaises(RuntimeError, r._reduction_steps[0].execute, (r, "test") )
            
    def test_data_files(self):
        r = Reducer()
        r.append_data_file("AsciiExample.txt")
                
        # Check that we can empty the list of data files
        self.assertEqual(len(r._data_files), 1)
        r.clear_data_files()
        self.assertEqual(len(r._data_files), 0)

    def test_imports(self):
        import reduction_gui
        import reduction_application
        

if __name__ == '__main__':
    unittest.main()