# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import UnitFactory, Unit, Label, UnitLabel
import types

class UnitsTest(unittest.TestCase):

    def test_Label_is_returned_from_Factory(self):
        label_unit = UnitFactory.Instance().create("Label")
        self.assertTrue(isinstance(label_unit, Unit))
        self.assertTrue(isinstance(label_unit, Label))
        label_unit.setLabel("Temperature", "K")
        self.assertEquals("Temperature", label_unit.caption())
        self.assertEquals("K", label_unit.label())
        self.assertTrue(isinstance(label_unit.symbol(), UnitLabel))

    def test_quick_conversion_with_string_input(self):
        energy = UnitFactory.Instance().create("Energy")
        factor, power = energy.quickConversion("Wavelength")
        self.assertAlmostEquals(factor, 9.04456756843)
        self.assertEquals(power, -0.5)

    def test_quick_conversion_with_unit_input(self):
        energy = UnitFactory.Instance().create("Energy")
        wavelength = UnitFactory.Instance().create("Wavelength")
        factor, power = energy.quickConversion(wavelength)
        self.assertAlmostEquals(factor, 9.04456756843)
        self.assertEquals(power, -0.5)

# -------------  Failure cases  -------------------
    def test_failure_quick_conversion_failure_with_same_input(self):
        energy = UnitFactory.Instance().create("Energy")
        self.assertRaises(RuntimeError, energy.quickConversion, "Energy")


if __name__ == '__main__':
    unittest.main()
