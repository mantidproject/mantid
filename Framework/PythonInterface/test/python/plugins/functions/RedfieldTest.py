# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class RedfieldTest(unittest.TestCase):

	def test_function_has_been_registered(self):
		status, msg = is_registered("Redfield")
		if not status:
			self.fail(msg)

	def test_function_output(self):
		input = np.linspace(0, 1000, 4)  # Between 0-1000 Gauss
		expected = [0.91847597, 0.76278678, 0.5056509, 0.3237545]
		tolerance = 1.0e-05
		function_params = {"A0": 10, "Hloc": 50, "Tau": 0.1}
		status, output = check_output("Redfield", input, expected, tolerance, **function_params)
		if not status:
			msg = 'Computed output {} from input {} unequal to expected: {}'
			self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

	def test_do_fit(self):
		do_a_fit(np.arange(0.1, 16, 0.2), 'Redfield', guess = dict(A0 = 0.2, Hloc = 0.1, Tau = 0.1),target = dict(A0 = 0.2, Hloc = 0.1, Tau = 0.1), atol = 0.01)


if __name__ == '__main__':
	unittest.main()
