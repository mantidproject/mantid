from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class ChudleyElliotTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("ChudleyElliot")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [4.52422472e-05, 4.51112743e-03,
                    3.37001059e-01, 4.78915177e-01]
        tolerance = 1.0e-04
        status, output = check_output("ChudleyElliot", input, expected,
                                      tolerance, Tau=1.42, L=2.42)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    @staticmethod
    def test_do_fit():
        do_a_fit(np.asarray([0.01, 0.1, 1.0, 10.0]), 'ChudleyElliot',
                 guess=dict(Tau=1.0, L=1.0),
                 target=dict(Tau=1.42, L=2.42), atol=0.01)


if __name__ == '__main__':
    unittest.main()
