# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit
from MsdTestHelper import (is_registered, check_output, do_a_fit)

class BesselTest(unittest.TestCase):
	def test_function_has_been_registered(self):
		status, msg = is_registered("Bessel")
		if not status:
			self.fail(msg)

	def test_function_output(self):
		input = [0.0, 4.0, 8.0, 12.0]
		expected = [0.09900249722395764, -0.010116392139648565, -0.024891569859118595, -0.006976567202361606]
		tolerance = 1.0e-05
		status, output = check_output("Bessel", input, expected, tolerance, A0=0.1, fi=0.2, nu=0.2)
		if not status:
			msg = 'Computed output {} from input {} unequal to expected: {}'
			self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

	def test_do_fit(self):
		do_a_fit(np.arange(0.1, 16, 0.2), 'Bessel',guess=dict(A0=0.15, phi=0.25, nu=0.25),target=dict(A0=0.1, fi=0.2, nu=0.2), atol=0.01)

if __name__ == '__main__':
	unittest.main()
