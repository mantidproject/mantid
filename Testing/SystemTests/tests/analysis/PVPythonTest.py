#pylint: disable=W0232,R0903
import stresstesting
from paraview.simple import *


class PVPythonTest(stresstesting.MantidStressTest):

    def runTest(self):
        self.assertEqual(GetParaViewVersion().major, 5)
