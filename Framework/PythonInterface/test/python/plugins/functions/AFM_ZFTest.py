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


class AFMZFTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("AFMZF")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [0.2, 0.16785953164092032, 0.11526697933333936, 0.08922342844371162]
        tolerance = 1.0e-05
        status, output = check_output("AFMZF", input, expected, tolerance, A0 = 0.2, Freq = 1, Angle = 50, Sigma = 0.2, Phi = 0.0)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(A0 = 0.25, Freq = 1.2, Angle = 50, Sigma = 10.5, Phi = 0.0)
        target = dict(A0 = 0.2, Freq = 1, Angle = 50, Sigma = 0.2, Phi = 0.0)
        do_a_fit(np.arange(0.1, 16, 0.2), 'AFMZF', guess, target, atol = 0.01)

if __name__ == '__main__':
	unittest.main()
