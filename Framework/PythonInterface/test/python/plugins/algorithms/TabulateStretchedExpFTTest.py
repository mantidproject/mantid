#import mantid
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import TabulateStretchedExpFT
import unittest
from pdb import set_trace as tr

class TabulateStretchedExpFTTest(unittest.TestCase):

    def testWrongInput(self):
        """Pass input out of bouns"
        """
        self.assertRaises(ValueError,
                          TabulateStretchedExpFT,
                          BetaValues=[-0.1, 0.1, 0.3])
        self.assertRaises(ValueError,
                          TabulateStretchedExpFT,
                          EvalN=1)
        self.assertRaises(ValueError,
                          TabulateStretchedExpFT,
                          Taumax=1E+09)
        self.assertRaises(ValueError,
                          TabulateStretchedExpFT,
                          Emax=-0.1)

    def testEvaluation(self):
        """Compare tables in the WorkspaceGroup against "Golden data"
        """
        self._tables = TabulateStretchedExpFT(BetaValues=[0.2, 0.1, 0.3],
                                             EvalN=2, Taumax=1000.0, Emax=1.0,
                                             OutputWorkspace="tables")
        function_values = self._tables.getItem(0).dataY(0)
        self.assertAlmostEqual(function_values[0], 1.19785063e+02, places=5)
        self.assertAlmostEqual(function_values[1], 1.05652325e-04, places=9)
        AnalysisDataService.remove(self._tables.getName())

if __name__ == '__main__':
    unittest.main()
