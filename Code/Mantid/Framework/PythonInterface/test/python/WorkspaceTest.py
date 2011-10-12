import unittest
import sys
from testhelpers import run_algorithm

from mantid.api import algorithm_mgr, Workspace, WorkspaceProperty_Workspace

class WorkspaceTest(unittest.TestCase):
  
    def test_that_one_cannot_be_instantiated(self):
        try:
            Workspace()
            error = False
        except RuntimeError: # For some reason self.assertRaises doesn't catch this
            error = True
        self.assertTrue(error, True)

    def test_that_alg_get_property_is_converted_correctly(self):
        nspec = 2
        wsname = 'LOQ48127' 
        alg = run_algorithm('LoadRaw', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=nspec, child=True)
        ws_prop = alg.get_property('OutputWorkspace')
        self.assertEquals(type(ws_prop), WorkspaceProperty_Workspace)
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(ws_prop.value, Workspace))
        mem = ws_prop.value.get_memory_size()
        self.assertTrue( (mem > 0) )
        
    # Disabled until the get/set property stuff in IPropertyManager can be a little more forgiving.
    def xtest_ws_as_input_to_algorithm(self): 
        alg = run_algorithm('LoadRaw', Filename='LOQ48127.raw', OutputWorkspace='LOQ48127', SpectrumMax=1, child=True)
        ws = alg.get_property('OutputWorkspace').value
        alg = run_algorithm('ConvertUnits', Target='dSpacing', InputWorkspace=ws, OutputWorkspace=ws, child=True)