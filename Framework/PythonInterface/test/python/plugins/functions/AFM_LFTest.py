# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class AFM_LFTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("AFM_LF")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [0.2, 0.015066266766824094, 0.04206038804127508, 0.1938030263224746]
        tolerance = 1.0e-05
        status, output = check_output("AFM_LF", input, expected, tolerance, A0 = 0.2, Freq = 2, Angle = 50, Field = 10, Phi = 0.0)

        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(A0 = 0.25, Freq = 2.5, Angle = 50.5, Field = 11, Phi = 0.0)
        target = dict(A0 = 0.2, Freq = 2, Angle = 50, Field = 10, Phi = 0.0)
        do_a_fit(np.arange(0.1, 16, 0.2), 'AFM_LF', guess, target, atol = 0.01)

if __name__ == '__main__':
	unittest.main()
