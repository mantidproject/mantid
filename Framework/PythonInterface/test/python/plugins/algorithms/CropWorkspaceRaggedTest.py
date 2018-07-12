from __future__ import (absolute_import, division, print_function)

import unittest
import mantid.simpleapi as api
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
from numpy import nan as np_nan
from numpy import inf as np_inf
try:
    from math import nan as math_nan
except ImportError:
    math_nan = np_nan # rhel7 python is too old


class CropWorkspaceRaggedTest(unittest.TestCase):

    def test_nomad_inplace(self):
        api.LoadNexusProcessed(Filename='NOM_91796_banks.nxs', OutputWorkspace='NOM_91796_banks')
        alg_test = run_algorithm('CropWorkspaceRagged',
                                 InputWorkspace='NOM_91796_banks', OutputWorkspace='NOM_91796_banks',
                                 XMin=[0.67, 1.20, 2.42, 3.70, 4.12, 0.39],
                                 XMax=[10.20, 20.8, np_nan, math_nan, np_nan, 9.35])

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve('NOM_91796_banks')
        for i, Xlen in enumerate([477,981,1880,1816,1795,448]):
            self.assertEqual(len(outputws.readX(i)), Xlen)

        AnalysisDataService.remove('NOM_91796_banks')

    def test_nomad_no_mins(self):
        api.LoadNexusProcessed(Filename='NOM_91796_banks.nxs', OutputWorkspace='NOM_91796_banks')
        alg_test = run_algorithm('CropWorkspaceRagged',
                                 InputWorkspace='NOM_91796_banks', OutputWorkspace='NOM_91796_banks',
                                 XMax=[10.20, 20.8, np_inf, math_nan, np_nan, 9.35])

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve('NOM_91796_banks')
        for i, Xlen in enumerate([511,1041,2001,2001,2001,468]): # larger than in test_nomad_inplace
            self.assertEqual(len(outputws.readX(i)), Xlen)

        AnalysisDataService.remove('NOM_91796_banks')

    def test_sample_workspace(self):
        # numpy 1.7 (on rhel7) doesn't have np.full
        xmins = np.array([2500.]*200)
        xmins[11] = 3100.
        xmaxs = np.array([5500.]*200)
        xmaxs[12] = 4700.
        ws = api.CreateSampleWorkspace()
        cropped = api.CropWorkspaceRagged(ws, XMin=xmins, XMax=xmaxs)

        self.assertEqual(cropped.getNumberHistograms(), 200)
        for i in range(cropped.getNumberHistograms()):
            if i == 11:
                self.assertEqual(cropped.readX(i).size, 12)
            elif i == 12:
                self.assertEqual(cropped.readX(i).size, 11)
            else:
                self.assertEqual(cropped.readX(i).size, 15)

if __name__ == '__main__':
    unittest.main()
