# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
from mantid.simpleapi import MDNormSCDPreprocessIncoherent


class MDNormSCDPreprocessIncoherentTest(unittest.TestCase):
    def testCNCS(self):
        # CNCS_7860 is not an incoherent scatterer but for this test
        # it doesn't matter
        SA, Flux = MDNormSCDPreprocessIncoherent(Filename='CNCS_7860',
                                                 MomentumMin=1,
                                                 MomentumMax=1.5)

        # Just compare 10 points of the Flux
        flux_cmp = np.array([0.00000000e+00, 7.74945234e-04, 4.96143098e-03,
                             1.18914010e-02, 1.18049991e-01, 7.71872176e-01,
                             9.93078957e-01, 9.96312349e-01, 9.98450129e-01,
                             1.00000002e+00])
        np.testing.assert_allclose(Flux.extractY()[0][::1000], flux_cmp)
        self.assertEqual(Flux.getXDimension().name, 'Momentum')
        self.assertEqual(Flux.getXDimension().getUnits(), 'Angstrom^-1')
        self.assertEqual(Flux.blocksize(), 10000)
        self.assertEqual(Flux.getNumberHistograms(), 1)

        # Compare every 20-th bin of row 64
        SA_cmp = np.array([0.11338311, 0.18897185, 0.15117748, 0.11338311, 0.03779437,
                           0.07558874, 0.15117748, 0.18897185, 0.03779437, 0.15117748,
                           0.11338311, 0.07558874, 0.03779437, 0.        , 0.56691555,
                           0.26456059, 0.11338311, 0.07558874, 0.11338311, 0.])
        np.testing.assert_allclose(SA.extractY().reshape((-1,128))[::20,64], SA_cmp)
        self.assertEqual(SA.getXDimension().name, 'Momentum')
        self.assertEqual(SA.getXDimension().getUnits(), 'Angstrom^-1')
        self.assertEqual(SA.blocksize(), 1)
        self.assertEqual(SA.getNumberHistograms(), 51200)
        self.assertEqual(SA.getNEvents(), 51200)


if __name__=="__main__":
    unittest.main()
