# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from abins.instruments.instrument import Instrument


class InstrumentTest(unittest.TestCase):
    def test_instrument_notimplemented(self):
        instrument = Instrument()

        with self.assertRaises(NotImplementedError):
            instrument.calculate_q_powder()

        with self.assertRaises(NotImplementedError):
            instrument.convolve_with_resolution_function()


if __name__ == '__main__':
    unittest.main()
