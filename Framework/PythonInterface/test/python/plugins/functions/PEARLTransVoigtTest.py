# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 20120 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, do_a_fit, check_output


class PEARLTransVoigtTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("PEARLTransVoigt")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [1092, 1094, 1096, 1098]
        expected = [14.106022, 14.074826, 14.062504, 14.069141]
        tolerance = 1.0e-05
        status, output = check_output("PEARLTransVoigt",
                                      input,
                                      expected,
                                      tolerance,
                                      Position=1096.3,
                                      LorentzianFWHM=45.8,
                                      GaussianFWHM=25.227,
                                      Bg0=25.0,
                                      Bg1=0.015,
                                      Bg2=0.0)
        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        do_a_fit(np.arange(0.1, 16, 0.2),
                 'PEARLTransVoigt',
                 guess=dict(Position=1096, LorentzianFWHM=45.5, GaussianFWHM=25.4, Bg0=25.2, Bg1=0.017, Bg2=0.0),
                 target=dict(Position=1096.3, LorentzianFWHM=45.8, GaussianFWHM=25.227, Bg0=25.0, Bg1=0.015, Bg2=0.0),
                 atol=0.01)


if __name__ == '__main__':
    unittest.main()
