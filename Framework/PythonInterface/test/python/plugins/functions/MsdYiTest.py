from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory

class MsdYiTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        try:
            FunctionFactory.createFunction("MsdYi")
        except RuntimeError as exc:
            self.fail("Could not create MsdYi function: %s" % str(exc))

    def test_function_output(self):
        msd_yi = FunctionWrapper("MsdYi", Height=1.0, MSD=0.05, Sigma=1.0)
        input = np.array([[1, 2], [3, 4]])
        output = msd_yi(input)
        self.assertAlmostEqual(output[0][0], 0.036)
        self.assertAlmostEqual(output[0][1], 0.531)
        self.assertAlmostEqual(output[1][0], 1.467)
        self.assertAlmostEqual(output[1][1], 3.699)
