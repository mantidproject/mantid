from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from MsdTestHelper import is_registered, check_output

class MsdGaussTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("MsdGauss")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = np.array([[0, 1], [2, 3]])
        expected = np.array([1.000, 0.951], [0.819, 0.638])
        tolerance = 0.0005
        status, output = check_output("MsdGauss", input, expected, tolerance, Height=1.0, MSD=0.05)

        if not status:
            self.fail("Computed output " + str(output) + " from input " + str(input) + " is not"
                      " equal to the expected output: " + str(expected))
