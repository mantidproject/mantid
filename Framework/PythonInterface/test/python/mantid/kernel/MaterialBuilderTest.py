# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import MaterialBuilder, NumberDensityUnit


class MaterialBuilderTest(unittest.TestCase):

    def test_build_material(self):
        builder = MaterialBuilder()
        material = builder.setName('Bizarre oxide').setFormula('Al2 O3').setNumberDensity(0.23).build()
        self.assertEqual(material.name(), 'Bizarre oxide')
        formula = material.chemicalFormula()
        self.assertEqual(len(formula), 2)
        atoms = formula[0]
        multiplicities = formula[1]
        self.assertEqual(len(atoms), 2)
        self.assertEqual(atoms[0].symbol, 'Al')
        self.assertEqual(atoms[1].symbol, 'O')
        self.assertEqual(len(multiplicities), 2)
        self.assertEqual(multiplicities[0], 2)
        self.assertEqual(multiplicities[1], 3)
        self.assertEqual(material.numberDensity, 0.23)

    def test_number_density_units(self):
        builder = MaterialBuilder()
        builder.setName('Bizarre oxide').setFormula('Al2 O3').setNumberDensity(0.23)
        builder.setNumberDensityUnit(NumberDensityUnit.FormulaUnits)
        material = builder.build()
        self.assertEqual(material.numberDensity, 0.23 * (2. + 3.))


if __name__ == '__main__':
    unittest.main()
