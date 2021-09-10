# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from abins.constants import ALL_INSTRUMENTS
from abins.instruments.instrument import Instrument


class InstrumentTest(unittest.TestCase):
    def test_instrument_notimplemented(self):
        instrument = Instrument()

        with self.assertRaises(NotImplementedError):
            instrument.calculate_q_powder()

        with self.assertRaises(NotImplementedError):
            instrument.convolve_with_resolution_function()


class GetInstrumentTest(unittest.TestCase):
    def setUp(self):
        ALL_INSTRUMENTS.append('Unimplemented')

    def tearDown(self):
        ALL_INSTRUMENTS.pop()

    def test_instrument_notfound(self):
        from abins.instruments import get_instrument

        with self.assertRaises(ValueError):
            get_instrument('Unheardof')
        with self.assertRaises(NotImplementedError):
            get_instrument('Unimplemented')


if __name__ == '__main__':
    unittest.main()
