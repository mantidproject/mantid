from __future__ import (absolute_import, division, print_function)
#from six.moves import range

import unittest
#import numpy
import mantid.simpleapi as api
#from mantid.kernel import *
#from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class CropWorkspaceRaggedTest(unittest.TestCase):

    def test_nomad_inplace(self):
        import numpy as np
        import math
        api.LoadNexusProcessed(Filename='NOM_91796_banks.nxs', OutputWorkspace='NOM_91796_banks')
        alg_test = run_algorithm('CropWorkspaceRagged',
                                 InputWorkspace='NOM_91796_banks', OutputWorkspace='NOM_91796_banks',
                                 Xmin=[0.67, 1.20, 2.42, 3.70, 4.12, 0.39],
                                 Xmax=[10.20, 20.8, np.nan, math.nan, np.nan, 9.35])

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve('NOM_91796_banks')
        for i, Xlen in enumerate([477,981,1880,1816,1795,448]):
            self.assertEqual(len(outputws.readX(i)), Xlen)

        AnalysisDataService.remove('NOM_91796_banks')

        return


if __name__ == '__main__':
    unittest.main()
