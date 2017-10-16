from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory

class MsdPetersTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        try:
            FunctionFactory.createFunction("MsdPeters")
        except RuntimeError as exc:
            self.fail("Could not create MsdPeters function: %s" % str(exc))

    def test_function_output(self):
        msd_peters = FunctionWrapper("MsdPeters", Height=1.0, MSD=0.05, Beta=1.0)
        input = np.array([[0, 1], [2, 3]])
        output = msd_peters(input)
        self.assertAlmostEqual(output[0][0], 1.000)
        self.assertAlmostEqual(output[0][1], 0.992)
        self.assertAlmostEqual(output[1][0], 0.968)
        self.assertAlmostEqual(output[1][1], 0.930)
