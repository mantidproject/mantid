# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import UnitFactory, UnitFactoryImpl, Unit

class UnitFactoryTest(unittest.TestCase):

    def test_alias_is_of_type_UnitFactoryImpl(self):
        self.assertTrue(isinstance(UnitFactory, UnitFactoryImpl))

    def test_known_unit_can_be_created(self):
        energy = UnitFactory.create("Energy")
        self.assertTrue(isinstance(energy, Unit))

    def test_unknown_unit_raises_error(self):
        self.assertRaises(RuntimeError, UnitFactory.create,
                          "NotAUnit")

    def test_keys_returns_a_non_empty_python_list_of_unit_keys(self):
        known_units = UnitFactory.getKeys()

        self.assertEquals(type(known_units), list)
        # Check length is at least the known core units
        # but allow for others to be added
        core_units = ['Empty', 'Label', 'TOF', 'Wavelength','Energy',
                      'Energy_inWavenumber', 'dSpacing', 'MomentumTransfer',
                      'QSquared', 'DeltaE', 'DeltaE_inWavenumber',
                      'DeltaE_inFrequency', 'Momentum', 'dSpacingPerpendicular']
        self.assertLessEqual(len(core_units), len(known_units))

        for unit in core_units:
            self.assertTrue(unit in known_units, "%s unit not found in UnitFactory keys" % unit)

if __name__ == '__main__':
    unittest.main()
