from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, create_model, create_test_workspace, create_function_string
from mantid.simpleapi import Fit


class MsdGaussTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("MsdGauss")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = np.array([[0, 1], [2, 3]])
        expected = np.array([[1., 0.95122942], [0.81873075, 0.63762815]])
        tolerance = 0.000001
        status, output = check_output("MsdGauss", input, expected, tolerance, Height=1.0, MSD=0.05)

        if not status:
            self.fail("Computed output " + str(output) + " from input " + str(input) +
                      " is not equal to the expected output: " + str(expected))

    def test_use_in_fit(self):
        workspace = create_test_workspace(create_model("MsdGauss", Height=1.0, MSD=0.05), 1000)
        function_string = create_function_string("MsdGauss", Height=1.0, MSD=0.05)
        Fit(Function=function_string, InputWorkspace=workspace, StartX=1.2, EndX=1200)


if __name__ == '__main__':
    unittest.main()
