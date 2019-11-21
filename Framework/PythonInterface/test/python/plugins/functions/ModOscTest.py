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


class ModOscTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("ModOsc")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [0.933962026044940,0.0630575696651452, -0.141595984565378, -0.00124998969353344]
        tolerance = 1.0e-05
        status, output = check_output("ModOsc", input, expected, tolerance, A0=1.0, Freq=0.1, ModFreq=0.2, Phi=0.3)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(A0=0.25, Freq=1.15, ModFreq=0.15, Phi=0.0)
        target = dict(A0=0.2, Freq=1.0, ModFreq=0.1, Phi=0.0)
        do_a_fit(np.arange(0.1, 16, 0.2), 'ModOsc', guess, target, atol=0.01)


if __name__ == '__main__':
    unittest.main()
