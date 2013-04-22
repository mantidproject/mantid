import unittest

import mantidsimple

import os
import sys

#======================================================================================================================
# Helper class for test
class TemporaryPythonAlgorithm(object):
    """
    Dumps the given code to a file in the Python algorithm directory
    an removes the file in the del method
    """
    def __init__(self, name, code):
        plugin_dirs = mantidsimple.mtd.settings['python.plugins.directories'].split(";")
        if len(plugin_dirs) == 0:
            raise RuntimeError("No Python algorithm directories defined")
        
        self._pyfile = os.path.join(plugin_dirs[0], name + ".py")
        alg_file = open(self._pyfile, "w")
        alg_file.write(code)
        alg_file.close()
        
    def __del__(self):
        try:
            os.remove(self._pyfile)
            pycfile = self._pyfile.replace(".py",".pyc")
            os.remove(pycfile)
        except OSError:
            pass

#======================================================================================================================

class SimpleAPITest(unittest.TestCase):

    def test_version_number_equals_1(self):
        self.assertEquals(mantidsimple.apiVersion(), 1)
    
    def test_alg_has_expected_doc_string(self):
        # Test auto generated string, Load is manually written
        expected_doc = \
        """Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.

Property descriptions: 

InputWorkspace(Input:req) *MatrixWorkspace*       Workspace containing the input data

OutputWorkspace(Output:req) *MatrixWorkspace*       The name to give the output workspace

Params(Input:req) *dbl list*       A comma separated list of first bin boundary, width, last bin boundary. Optionally this can be followed by a comma and more widths and last boundary pairs. Optionally this can also be a single number, which is the bin width. In this case, the boundary of binning will be determined by minimum and maximum TOF values among all events, or previous binning boundary, in case of event Workspace, or non-event Workspace, respectively. Negative width values indicate logarithmic binning. 

PreserveEvents(Input) *boolean*       Keep the output workspace as an EventWorkspace, if the input has events (default). If the input and output EventWorkspace names are the same, only the X bins are set, which is very quick. If false, then the workspace gets converted to a Workspace2D histogram.
"""
        doc = mantidsimple.rebin.__doc__
        self.assertTrue(len(doc) > 0 )
        self.assertEquals(doc, expected_doc)
        
    def test_function_call_executes_correct_algorithm_when_passed_correct_args(self):
        wsname = 'test_function_call_executes_correct_algorithm_when_passed_correct_args'
        data = [1.0,2.0,3.0,4.0,5.0]
        mantidsimple.CreateWorkspace(DataX=data,DataY=data,OutputWorkspace=wsname,NSpec=1,UnitX='Wavelength')
        self.assertTrue( wsname in mantidsimple.mtd )
        
    def test_function_call_raises_ValueError_when_passed_args_with_invalid_values(self):
        # lhs code bug means we can't do this "self.assertRaises(simpleapi.LoadNexus, 'DoesNotExist')" --> ticket #4186
        try:
            mantidsimple.LoadNexus(Filename='DoesNotExist.nxs')
            self.fail("A ValueError was not thrown")
        except ValueError:
            pass
        
    def test_function_call_raises_AttributeError_when_passed_incorrect_args(self):
        try:
            mantidsimple.LoadNexus(NotAProperty=1)
            self.fail("An AttributeError was not thrown for an incorrect argument")
        except AttributeError:
            pass

        
    def test_that_dialog_call_raises_runtime_error(self):
        try:
            mantidsimple.LoadEventNexusDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong")
                
    def test_python_alg_can_use_other_python_alg_through_simple_api(self):
        """
        Runs a test in a separate process as it requires a reload of the
        whole mantid module 
        """
        src = """
from MantidFramework import PythonAlgorithm, mtd
from mantidsimple import *

class %(name)s(PythonAlgorithm):

    def PyInit(self):
        pass
    def PyExec(self):
        %(execline)s
        
mtd.registerPyAlgorithm(%(name)s())
"""
        name1 = "MantidSimplePythonAlgorithm1"
        name2 = "MantidSimplePythonAlgorithm2"
        src1 = src % {"name":name1,"execline":name2+"()"}
        src2 = src % {"name":name2,"execline":"pass"}
        a = TemporaryPythonAlgorithm(name1,src1)
        b = TemporaryPythonAlgorithm(name2,src2)
        import subprocess
        # Try to use algorithm 1 to run algorithm 2
        cmd = sys.executable + ' -c "from MantidFramework import *;mtd.initialise();from mantidsimple import %(name)s;%(name)s()"' % {'name':name1}
        try:
            subprocess.check_call(cmd,shell=True)
        except subprocess.CalledProcessError,exc:
            self.fail("Error occurred running one Python algorithm from another: %s" % str(exc))
        
        # Ensure the files are removed promptly
        del a,b

if __name__ == '__main__':
    unittest.main()
