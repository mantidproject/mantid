"""Defines tests for the WorkspaceProperty types within
Python algorithms
"""
import unittest
from mantid.api import PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction

class PythonAlgorithmWorkspacePropertyTest(unittest.TestCase):

    def _do_test(self, classtype):
        """Perform the test for the given type

            @param classtype :: The property class to declare
        """
        class WorkspaceProperties(PythonAlgorithm):

            _testdocstring = 'This is a workspace property'
            def PyInit(self):
                self.declareProperty(classtype("NoDocString", "", Direction.Input))
                self.declareProperty(classtype("WithDocString", "", Direction.Input), self._testdocstring)

            def PyExec(self):
                pass
        #######################################################
        alg = WorkspaceProperties()
        alg.initialize()
        props = alg.getProperties()
        self.assertEquals(2, len(props))

        nodoc = alg.getProperty("NoDocString")
        self.assertTrue(isinstance(nodoc, classtype))
        self.assertEquals("", nodoc.documentation)
        withdoc = alg.getProperty("WithDocString")
        self.assertTrue(isinstance(withdoc, classtype))
        self.assertEquals(alg._testdocstring, withdoc.documentation)

    def test_alg_accepts_WorkspaceProperty_declaration(self):
        """Runs test for a general WorkspaceProperty
        """
        self._do_test(WorkspaceProperty)

if __name__ == "__main__":
    unittest.main()
