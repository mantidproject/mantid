import unittest
from mantid.api import (AlgorithmFactory, AlgorithmProxy, IAlgorithm, IEventWorkspace, ITableWorkspace,
                        PythonAlgorithm, MatrixWorkspace, mtd, WorkspaceGroup)
import mantid.simpleapi as simpleapi
import numpy

#======================================================================================================================

class SimpleAPITest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()
        
    def test_version_number_equals_2(self):
        self.assertEquals(simpleapi.apiVersion(), 2)
    
    def test_module_dict_seems_to_be_correct_size(self):
        # Check that the module has at least the same number
        # of attributes as unique algorithms
        module_dict = dir(simpleapi)
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue( len(module_dict) > len(all_algs) )
        
    def test_alg_has_expected_doc_string(self):
        # Test auto generated string, Load is manually written
        expected_doc = \
        """Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.

Property descriptions: 

InputWorkspace(Input:req) *MatrixWorkspace*       Workspace containing the input data

OutputWorkspace(Output:req) *MatrixWorkspace*       The name to give the output workspace

Params(Input:req) *dbl list*       A comma separated list of first bin boundary, width, last bin boundary. Optionally this can be followed by a comma and more widths and last boundary pairs. Optionally this can also be a single number, which is the bin width. In this case, the boundary of binning will be determined by minimum and maximum TOF values among all events, or previous binning boundary, in case of event Workspace, or non-event Workspace, respectively. Negative width values indicate logarithmic binning. 

PreserveEvents(Input) *boolean*       Keep the output workspace as an EventWorkspace, if the input has events. If the input and output EventWorkspace names are the same, only the X bins are set, which is very quick. If false, then the workspace gets converted to a Workspace2D histogram.

FullBinsOnly(Input) *boolean*       Omit the final bin if it's width is smaller than the step size
"""
        doc = simpleapi.rebin.__doc__
        self.assertTrue(len(doc) > 0 )
        self.assertEquals(doc, expected_doc)
        
    def test_function_call_executes_correct_algorithm_when_passed_correct_args(self):
        wsname = 'test_function_call_executes_correct_algorithm_when_passed_correct_args'
        data = [1.0,2.0,3.0,4.0,5.0]
        simpleapi.CreateWorkspace(data,data,OutputWorkspace=wsname,NSpec=1,UnitX='Wavelength')
        self.assertTrue( wsname in mtd )
        
    def test_function_call_executes_with_output_workspace_on_lhs(self):
        data = [1.0,2.0,3.0,4.0,5.0]
        wavelength = simpleapi.CreateWorkspace(data,data,NSpec=1,UnitX='Wavelength')
        wsname = 'wavelength'
        self.assertTrue( wsname in mtd )
        
    def test_function_call_executes_when_algorithm_has_only_inout_workspace_props(self):
        data = [1.0,2.0,3.0,4.0,5.0, 6.0]
        wavelength = simpleapi.CreateWorkspace(data,data,NSpec=3,UnitX='Wavelength')
        simpleapi.MaskDetectors(wavelength,WorkspaceIndexList=[1,2])
        
    def test_function_accepts_EnableLogging_keyword(self):
        # The test here is that the algorithm runs without falling over about the EnableLogging keyword being a property
        wsname = 'test_function_call_executes_correct_algorithm_when_passed_correct_args'
        data = [1.0,2.0,3.0,4.0,5.0]
        simpleapi.CreateWorkspace(data,data,OutputWorkspace=wsname,NSpec=1,UnitX='Wavelength',EnableLogging=False)
        self.assertTrue( wsname in mtd )
    
    def test_function_call_raises_ValueError_when_passed_args_with_invalid_values(self):
        # lhs code bug means we can't do this "self.assertRaises(simpleapi.LoadNexus, 'DoesNotExist')" --> ticket #4186
        try:
            simpleapi.LoadNexus(Filename='DoesNotExist.nxs')
            self.fail("A ValueError was not thrown")
        except ValueError:
            pass
        
    def test_function_call_raises_RuntimeError_when_passed_incorrect_args(self):
        try:
            simpleapi.LoadNexus(NotAProperty=1)
            self.fail("A RuntimeError was not thrown")
        except RuntimeError:
            pass

    def test_function_call_raises_RuntimeError_if_num_of_ret_vals_doesnt_match_num_assigned_vars(self):
        try:
            ws, ws2 = simpleapi.CreateWorkspace([1.5],[1.5],NSpec=1,UnitX='Wavelength')
        except RuntimeError, exc:
            # Check the error is correct and it's not some random runtime error
            if 'CreateWorkspace is trying to return 1 output(s) but you have provided 2 variable(s). These numbers must match.' == str(exc):
                pass
            else:
                self.fail("Exception was raised but it did not have the correct message: '%s'" % str(exc))
        
    def test_function_returns_correct_args_when_extra_output_props_are_added_at_execute_time(self):
        ws1 = simpleapi.CreateWorkspace([1.5],[1.5],NSpec=1,UnitX='Wavelength')
        ws2 = simpleapi.CreateWorkspace([1.5],[1.5],NSpec=1,UnitX='Wavelength')
        # RenameWorkspaces defines extra properties for each workspace renamed at runtime
        ws1a,ws2a = simpleapi.RenameWorkspaces(InputWorkspaces="ws1,ws2",WorkspaceNames="ws1a,ws2a")
        self.assertTrue(isinstance(ws1a, MatrixWorkspace))
        self.assertTrue(isinstance(ws2a, MatrixWorkspace))
        
    def test_function_uses_OutputWorkspace_keyword_over_lhs_var_name_if_provided(self):
        wsname = 'test_function_uses_OutputWorkspace_keyword_over_lhs_var_name_if_provided'
        data = [1.0,2.0,3.0,4.0,5.0]
        wkspace = simpleapi.CreateWorkspace(data,data,OutputWorkspace=wsname,NSpec=1,UnitX='Wavelength')
        self.assertTrue( wsname in mtd )
    
    def test_function_returns_only_mandatory_workspace_when_optional_output_is_not_given(self):
        _demo = simpleapi.CreateMDHistoWorkspace(SignalInput='1,2,3,4,5',ErrorInput='1,1,1,1,1',
                                                Dimensionality='1',Extents='-1,1',NumberOfBins='5',Names='A',Units='U')
        wsname = 'test_function_returns_only_mandatory_workspace_when_optional_output_is_not_given'
        query = simpleapi.QueryMDWorkspace(InputWorkspace=_demo,OutputWorkspace=wsname,MaximumRows='500',
                                           Normalisation='volume')

        self.assertTrue( isinstance(query, ITableWorkspace) )
        self.assertTrue( wsname in mtd )

    def test_function_returns_both_mandatory_and_optional_workspaces_when_optional_output_is_given(self):
        _demo = simpleapi.CreateMDWorkspace(Dimensions='2',EventType='MDEvent',Extents='1,10,1,10',Names='a,b',
                                            Units='MomentumTransfer,MomentumTransfer',SplitInto='4')
        wsname = 'test_function_returns_only_mandatory_workspace_when_optional_output_is_not_given'
        wsname_box = wsname + '_box'
        query = simpleapi.QueryMDWorkspace(InputWorkspace=_demo,OutputWorkspace=wsname,MaximumRows='500',
                                           Normalisation='volume',BoxDataTable=wsname_box)
        
        self.assertTrue( wsname in mtd )
        self.assertTrue( wsname_box in mtd )
        
        self.assertTrue( type(query) == tuple )
        self.assertEquals( 2, len(query) )
        
        self.assertTrue( isinstance(query[0], ITableWorkspace) )
        self.assertTrue( isinstance(query[1], ITableWorkspace) )

    def test_function_attached_as_workpace_method_is_attached_to_correct_types(self):
        self.assertTrue(hasattr(IEventWorkspace, "rebin"))
        self.assertTrue(hasattr(MatrixWorkspace, "rebin"))
        self.assertFalse(hasattr(ITableWorkspace, "rebin"))

    def test_function_attached_as_workpace_method_has_same_metainformation_as_free_function(self):
        self.assertEqual(MatrixWorkspace.rebin.__name__, simpleapi.Rebin.__name__)
        self.assertEqual(MatrixWorkspace.rebin.__doc__, simpleapi.Rebin.__doc__)
        
        # Signature of method will have extra self argument
        freefunction_sig = simpleapi.rebin.func_code.co_varnames
        expected_method_sig = ['self']
        expected_method_sig.extend(freefunction_sig)
        self.assertEqual(MatrixWorkspace.rebin.func_code.co_varnames, tuple(expected_method_sig))

    def test_function_attached_as_workpace_method_does_the_same_as_the_free_function(self):
        # Use Rebin as a test
        ws1 = simpleapi.CreateWorkspace(DataX=[1.5,2.0,2.5,3.0],DataY=[1,2,3],NSpec=1,UnitX='Wavelength')
        self.assertTrue(hasattr(ws1, "rebin"))

        ws2 = simpleapi.Rebin(ws1,Params=[1.5,1.5,3])
        ws3 = ws1.rebin(Params=[1.5,1.5,3])
        ws4 = ws1.rebin([1.5,1.5,3])
        result = simpleapi.CheckWorkspacesMatch(ws2,ws3)
        self.assertEquals("Success!",result)
        result = simpleapi.CheckWorkspacesMatch(ws2,ws4)
        self.assertEquals("Success!",result)

        simpleapi.DeleteWorkspace(ws1)
        simpleapi.DeleteWorkspace(ws2)
        simpleapi.DeleteWorkspace(ws3)

    def test_that_dialog_call_raises_runtime_error(self):
        try:
            simpleapi.LoadEventNexusDialog()
        except RuntimeError, exc:
            msg = str(exc)
            if msg != "Can only display properties dialog in gui mode":
                self.fail("Dialog function raised the correct exception type but the message was wrong")
                
    def test_call_inside_function_uses_new_variable_name(self):
        def rebin(workspace):
            # Should replace the input workspace
            workspace = simpleapi.Rebin(workspace, Params=[1,0.1,10])
            return workspace
        
        dataX=numpy.linspace(start=1,stop=3,num=11)
        dataY=numpy.linspace(start=1,stop=3,num=10)
        raw = simpleapi.CreateWorkspace(DataX=dataX,DataY=dataY,NSpec=1)
        raw = rebin(raw)
        # If this fails then the function above chose the name of the variable
        # over the actual object name
        self.assertTrue('workspace' in mtd)
        self.assertTrue('raw' in mtd)

    def test_python_alg_can_use_other_python_alg_through_simple_api(self):
        class SimpleAPIPythonAlgorithm1(PythonAlgorithm):
            def PyInit(self):
                pass
            def PyExec(self):
                from mantid.simpleapi import SimpleAPIPythonAlgorithm2
                SimpleAPIPythonAlgorithm2()
        class SimpleAPIPythonAlgorithm2(PythonAlgorithm):
            def PyInit(self):
                pass
            def PyExec(self):
                pass

        AlgorithmFactory.subscribe(SimpleAPIPythonAlgorithm1)
        AlgorithmFactory.subscribe(SimpleAPIPythonAlgorithm2)
        # ---------------------------------------------------------
        alg1 = SimpleAPIPythonAlgorithm1()
        alg1.initialize()
        # Puts function in simpleapi globals
        simpleapi_alg1_func = simpleapi._create_algorithm_function("SimpleAPIPythonAlgorithm1", 1, alg1)
        alg2 = SimpleAPIPythonAlgorithm1()
        alg2.initialize()
        # Puts function in simpleapi globals
        simpleapi._create_algorithm_function("SimpleAPIPythonAlgorithm2", 1, alg2)
        try:
            simpleapi_alg1_func()
        except RuntimeError, exc:
            self.fail("Running algorithm 2 from 1 failed: " + str(exc))

    def test_optional_workspaces_are_ignored_if_not_present_in_output_even_if_given_as_input(self):
        # Test algorithm
        from mantid.api import AlgorithmManager,PropertyMode,PythonAlgorithm,MatrixWorkspaceProperty,WorkspaceFactory
        from mantid.kernel import Direction
        class OptionalWorkspace(PythonAlgorithm):
            def PyInit(self):
                self.declareProperty(MatrixWorkspaceProperty("RequiredWorkspace", "", Direction.Output))
                self.declareProperty(MatrixWorkspaceProperty("OptionalWorkspace", "", Direction.Output, PropertyMode.Optional))
 
            def PyExec(self):
                ws = WorkspaceFactory.create("Workspace2D", NVectors=1, YLength=1,XLength=1)
                ws.dataY(0)[0] = 5
                self.setProperty("RequiredWorkspace", ws)
                self.getLogger().notice("done!")
        AlgorithmFactory.subscribe(OptionalWorkspace)
        
        # temporarily attach it to simpleapi module
        name="OptionalWorkspace"
        algm_object = AlgorithmManager.createUnmanaged(name, 1)
        algm_object.initialize()
        simpleapi._create_algorithm_function(name, 1, algm_object) # Create the wrapper

        # Call with no optional output specified
        result = simpleapi.OptionalWorkspace(RequiredWorkspace="required")
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertAlmostEqual(5, result.readY(0)[0], places=12)
        mtd.remove("required")
        
        # Call with both outputs specified
        result = simpleapi.OptionalWorkspace(RequiredWorkspace="required",OptionalWorkspace="optional")
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertAlmostEqual(5, result.readY(0)[0], places=12)
        mtd.remove("required")
        
        # Tidy up simple api function
        del simpleapi.OptionalWorkspace

    def test_create_algorithm_object_produces_initialized_non_child_alorithm_outside_PyExec(self):
        alg = simpleapi._create_algorithm_object("Rebin")
        self._is_initialized_test(alg, 1, expected_class=AlgorithmProxy,
                                  expected_child=False)

    def test_create_algorithm_with_version_produces_initialized_alorithm(self):
        alg = simpleapi._create_algorithm_object("LoadRaw", 2)
        self._is_initialized_test(alg, 2, expected_class=AlgorithmProxy,
                                  expected_child=False)

    def test_create_algorithm_produces_child_inside_PyExec(self):
        # A small test class to have a PyExec method call the
        # algorithm creation
        class TestAlg(PythonAlgorithm):
            def PyInit(self):
                pass
            def PyExec(self):
                self.alg = simpleapi._create_algorithm_object("Rebin")
        # end
        top_level = TestAlg()
        top_level.PyExec()
        self._is_initialized_test(top_level.alg, 1, expected_class=IAlgorithm,
                                  expected_child=True)

    def _is_initialized_test(self, alg, version, expected_class, expected_child):
        self.assertTrue(alg.isInitialized())
        self.assertEquals(expected_child,alg.isChild())
        self.assertEquals(alg.version(), version)
        self.assertTrue(isinstance(alg, expected_class))

    def test_validate_inputs_with_errors_stops_algorithm(self):
        class ValidateInputsTest(PythonAlgorithm):
            def PyInit(self):
                self.declareProperty("Prop1", 1.0)
                self.declareProperty("Prop2", 2.0)
            def validateInputs(self):
                return {"Prop1":"Value is less than Prop2"}
            def PyExec(self):
                pass
        AlgorithmFactory.subscribe(ValidateInputsTest)
        # ---------------------------------------------------------
        alg_obj = ValidateInputsTest()
        alg_obj.initialize()
        
        simpleapi_func = simpleapi._create_algorithm_function("ValidateInputsTest", 1, alg_obj)
        # call
        self.assertRaises(RuntimeError, simpleapi_func, Prop1=2.5, Prop2=3.5)

if __name__ == '__main__':
    unittest.main()
