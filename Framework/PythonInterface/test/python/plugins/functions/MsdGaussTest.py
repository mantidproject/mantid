from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory

class MsdGaussTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        try:
            FunctionFactory.createFunction("MsdGauss")
        except RuntimeError as exc:
            self.fail("Could not create MsdGauss function: %s" % str(exc))

    def test_function_output(self):
        msd_gauss = FunctionWrapper("MsdGauss", Height=1.0, MSD=0.05)
        input = np.array([[0, 1], [2, 3]])
        output = msd_gauss(input)
        self.assertAlmostEqual(output[0][0], 1.000)
        self.assertAlmostEqual(output[0][1], 0.951)
        self.assertAlmostEqual(output[1][0], 0.819)
        self.assertAlmostEqual(output[1][1], 0.638)
