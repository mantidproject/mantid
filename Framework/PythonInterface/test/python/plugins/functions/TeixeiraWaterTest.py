from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class TeixeiraWaterTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("TeixeiraWater")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [2.46820217e-05, 2.45907320e-03,
                    1.79512344e-01, 6.41115514e-01]
        tolerance = 1.0e-05
        status, output = check_output("TeixeiraWater", input, expected,
                                      tolerance, Tau=1.0, L=1.5)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    def test_do_fit(self):
        do_a_fit(np.arange(0.1, 2.2, 0.2), 'TeixeiraWater',
                 guess=dict(Tau=2.0, L=1.0),
                 target=dict(Tau=1.0, L=3.5), atol=0.01)


if __name__ == '__main__':
    unittest.main()
