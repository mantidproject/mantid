# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from euphonic import ureg

from abins.constants import ALL_INSTRUMENTS
from abins.instruments import get_instrument
from abins.instruments.instrument import Instrument
from abins.instruments.pychop import validate_pychop_params


class InstrumentTest(unittest.TestCase):
    def test_instrument_notimplemented(self):
        instrument = Instrument()

        with self.assertRaises(NotImplementedError):
            instrument.calculate_q_powder()

        with self.assertRaises(NotImplementedError):
            instrument.convolve_with_resolution_function()


class GetInstrumentTest(unittest.TestCase):
    def setUp(self):
        ALL_INSTRUMENTS.append("Unimplemented")

    def tearDown(self):
        ALL_INSTRUMENTS.pop()

    def test_instrument_notfound(self):
        with self.assertRaises(ValueError):
            get_instrument("Unheardof")
        with self.assertRaises(NotImplementedError):
            get_instrument("Unimplemented")


class LagrangeResolutionTest(unittest.TestCase):
    def test_lagrange_resolution_limits(self):
        """Test Lagrange Cu(220) resolution function segments

        This resolution function consists of a flat segment and polynomial;
        make sure they are switching at the right place.

        The parameters
        # "low_energy_cutoff_meV": 25,
        # "low_energy_resolution_meV": 0.8,

        Mean that below an energy transfer of 25 meV the resolution
        (expressed as 2 sigma) equals 0.8 meV. (I.e. sigma = 0.4 meV)

        Abins internal units are wavenumber, not meV, so we use Euphonic's
        Pint unit registry to convert to wavenumber. (Rather than rely on the
        values in abins.constants, which are used in the implementation.)
        """
        lagrange = get_instrument("Lagrange", setting="Cu(220) (Lagrange)")

        # Cutoff is around 35 meV
        frequencies = ([30, 40] * ureg("meV")).to("1/cm", context="spectroscopy").magnitude
        sigma_below, sigma_above = lagrange.get_sigma(frequencies)
        self.assertAlmostEqual((sigma_below * ureg("1/cm")).to("meV").magnitude, 0.4, 6)

        self.assertGreater(sigma_above, sigma_below)


class DirectValidationTest(unittest.TestCase):
    def test_pychop_validation(self):
        """Test validation function used by Abins with pychop instruments

        If the wrong incident energy / chopper setting are used together, there
        is no transmission ans pychop will raise errors. The validator should
        catch this before performing intensity calculations.
        """

        # All good, empty dict is False-y
        self.assertFalse(
            validate_pychop_params(
                name="MARI",
                chopper="A",
                chopper_frequency="400",
                e_i="4000",
                energy_units="cm-1",
            )
        )

        # Frequency outside limits
        issues = validate_pychop_params(
            name="MARI",
            chopper="A",
            chopper_frequency="100000",
            e_i="4000",
            energy_units="meV",
        )
        self.assertIn("ChopperFrequency", issues)
        self.assertIn("Valid frequencies: ", issues["ChopperFrequency"])

        # Non-int-able frequency
        issues = validate_pychop_params(
            name="MARI",
            chopper="A",
            chopper_frequency="not_castable_to_int",
            e_i="4000",
            energy_units="meV",
        )
        self.assertIn("ChopperFrequency", issues)
        self.assertIn("could not cast to integer", issues["ChopperFrequency"])

        # Bad freq / e_i combination, no transmission
        issues = validate_pychop_params(
            name="MARI",
            chopper="A",
            chopper_frequency="400",
            e_i="10",
            energy_units="meV",
        )
        self.assertIn("ChopperFrequency", issues)
        self.assertIn("Use PyChop to identify a valid setting.", issues["ChopperFrequency"])


if __name__ == "__main__":
    unittest.main()
