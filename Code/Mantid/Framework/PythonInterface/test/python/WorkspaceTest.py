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
        wsname = 'ALF15739' 
        alg = run_algorithm('LoadRaw', Filename='ALF15739.raw', OutputWorkspace=wsname, SpectrumMax=nspec, child=True)
        ws_prop = alg.get_property('OutputWorkspace')
        self.assertEquals(type(ws_prop), WorkspaceProperty_Workspace)
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(ws_prop.value, Workspace))
        mem = ws_prop.value.get_memory_size()
        self.assertTrue( (mem > 0) )
        