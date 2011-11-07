import unittest

from testhelpers import run_algorithm

from mantid.api import MatrixWorkspace, WorkspaceProperty_Workspace, Workspace

class MatrixWorkspaceTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            wsname = 'LOQ48127'
            alg = run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=1, child=True)
            self.__class__._test_ws = alg.get_property('OutputWorkspace').value

    def test_that_one_cannot_be_instantiated_directly(self):
        try:
            MatrixWorkspace()
            error = False
        except RuntimeError: # For some reason self.assertRaises doesn't catch this
            error = True
        self.assertTrue(error, True)

    def test_meta_information(self):
        self.assertEquals(self._test_ws.id(), "Workspace2D")
        self.assertEquals(self._test_ws.get_name(), "")
        self.assertEquals(self._test_ws.get_title().rstrip(), "direct beam")#The title seems to have a newline in it
        self.assertEquals(self._test_ws.get_comment(), "")
        self.assertEquals(self._test_ws.is_dirty(), False)
        self.assertTrue(self._test_ws.get_memory_size() > 0.0)
        self.assertEquals(self._test_ws.threadsafe(), True)


    def test_that_a_histogram_workspace_is_returned_as_a_MatrixWorkspace_from_a_property(self):
        nspec = 1
        wsname = 'LOQ48127'
        alg = run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=nspec, child=True)
        ws_prop = alg.get_property('OutputWorkspace')
        self.assertEquals(type(ws_prop), WorkspaceProperty_Workspace)
        workspace = ws_prop.value
        # Is Workspace in the hierarchy of the value
        self.assertTrue(isinstance(workspace, Workspace))
        # Have got a MatrixWorkspace back and not just the generic interface
        self.assertEquals(type(workspace), MatrixWorkspace)
        mem = workspace.get_memory_size()
        self.assertTrue( (mem > 0) )

    # Disabled until the get/set property stuff in IPropertyManager can be a little more forgiving.
    def test_MatrixWorkspace_as_input_to_algorithm(self):
        alg = run_algorithm('LoadRaw', Filename='LOQ48127.raw', OutputWorkspace='LOQ48127', SpectrumMax=1, child=True)
        ws = alg.get_property('OutputWorkspace').value
        alg = run_algorithm('ConvertUnits', Target='dSpacing', InputWorkspace=ws, OutputWorkspace=ws, child=True)
        ws = alg.get_property('OutputWorkspace').value
        self.assertEquals(type(ws), MatrixWorkspace)
