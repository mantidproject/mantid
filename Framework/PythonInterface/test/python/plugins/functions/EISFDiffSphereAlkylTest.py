from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class EISFDiffSphereAlkylTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered('EISFDiffSphereAlkyl')
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [[0.99960073, 0.96138418, 0.47836973, 0.00162784]]
        tolerance = 1.0e-05
        status, output = check_output('EISFDiffSphereAlkyl', input, expected,
                                      tolerance, A=1.0, Rmin=0.5, Rmax=6.3)
        if not status:
            self.fail("Computed output " + str(output) + " from input " + str(input) +
                      " is not equal to the expected output: " + str(expected))

    def test_do_a_fit(self):
        do_a_fit(np.arange(0.1, 2.2, 0.2), 'EISFDiffSphereAlkyl',
                 guess=dict(A=2.0, Rmin=2.0, Rmax=3.0),
                 target=dict(A=1.0, Rmin=0.5, Rmax=6.3), atol=0.01)


if __name__ == '__main__':
    unittest.main()
