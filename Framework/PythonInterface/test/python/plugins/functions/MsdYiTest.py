from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from MsdTestHelper import is_registered, check_output

class MsdYiTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("MsdGauss")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = np.array([[1, 2], [3, 4]])
        expected = np.array([0.036, 0.531], [1.467, 3.699])
        tolerance = 0.0005
        status, output = check_output("MsdYi", input, expected, tolerance, Height=1.0, MSD=0.05, Sigma=1.0)

        if not status:
            self.fail("Computed output " + str(output) + " from input " + str(input) + " is not"
                      " equal to the expected output: " + str(expected))