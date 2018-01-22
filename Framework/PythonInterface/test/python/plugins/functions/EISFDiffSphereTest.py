from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class EISFDiffSphereTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("EISFDiffSphere")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [9.9975503e-01, 9.7575570e-01, 4.1939654e-02, 4.7662027e-06]
        tolerance = 1.0e-05
        status, output = check_output("EISFDiffSphere", input, expected,
                                      tolerance, A=1.0, R=3.5)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    def test_do_fit(self):
        do_a_fit(np.arange(0.1, 2.2, 0.2), 'EISFDiffSphere',
                 guess=dict(A=2.0, R=1.0),
                 target=dict(A=1.0, R=3.5), atol=0.01)


if __name__ == '__main__':
    unittest.main()
