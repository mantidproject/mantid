# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import MaterialBuilder, NumberDensityUnit

NAME = 'Bizarre oxide'
FORMULA = 'Al2 O3'
NUMBER_DENSITY = 0.23  # atoms / Angstrom^3
MASS_DENSITY = 3.987  # g/cm^3
PACKING_OBS = 0.51192  # packing fraction from the above two values


class MaterialBuilderTest(unittest.TestCase):
    def test_build_material(self):
        builder = MaterialBuilder()
        material = builder.setName(NAME).setFormula(FORMULA).setNumberDensity(NUMBER_DENSITY).build()
        self.assertEqual(material.name(), NAME)
        self.assertEqual(material.numberDensity, NUMBER_DENSITY)
        self.assertEqual(material.numberDensityEffective, NUMBER_DENSITY)
        self.assertEqual(material.packingFraction, 1.)

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
        self.assertEqual(material.numberDensity, NUMBER_DENSITY)

    def test_packing_fraction(self):
        builder = MaterialBuilder().setName(NAME).setFormula(FORMULA)

        # setting nothing should be an error
        try:
            material = builder.build()
            raise AssertionError('Should throw an exception')
        except RuntimeError as e:
            assert 'number density' in str(e)

        builder = MaterialBuilder().setName(NAME).setFormula(FORMULA).setNumberDensity(NUMBER_DENSITY)

        # only with number density
        material = builder.build()
        self.assertEqual(material.name(), NAME)
        self.assertEqual(material.numberDensity, material.numberDensityEffective)
        self.assertEqual(material.packingFraction, 1.0)
        self.assertEqual(material.numberDensity, NUMBER_DENSITY)

        # with number density and packing fraction
        material = builder.setPackingFraction(0.5).build()
        self.assertEqual(material.name(), NAME)
        self.assertEqual(material.numberDensity, NUMBER_DENSITY)
        self.assertEqual(material.numberDensityEffective, NUMBER_DENSITY * 0.5)
        self.assertEqual(material.packingFraction, 0.5)

        # start a brand new material builder
        builder = MaterialBuilder().setName(NAME).setFormula(FORMULA).setMassDensity(MASS_DENSITY)

        # only with mass density
        material = builder.build()
        self.assertEqual(material.name(), NAME)
        self.assertEqual(material.numberDensity, material.numberDensityEffective)
        self.assertEqual(material.packingFraction, 1.0)
        self.assertAlmostEqual(material.numberDensityEffective, NUMBER_DENSITY * PACKING_OBS, delta=1e-4)

        # with mass density and number density
        material = builder.setNumberDensity(NUMBER_DENSITY).build()
        self.assertEqual(material.name(), NAME)
        self.assertEqual(material.numberDensity, NUMBER_DENSITY)
        self.assertAlmostEqual(material.numberDensityEffective, NUMBER_DENSITY * PACKING_OBS, delta=1e-4)
        self.assertAlmostEqual(material.packingFraction, PACKING_OBS, delta=1e-4)

        # setting all 3 should be an error
        try:
            material = builder.setPackingFraction(0.5).setNumberDensity(NUMBER_DENSITY).setMassDensity(
                MASS_DENSITY).build()
            raise AssertionError('Should throw an exception')
        except RuntimeError as e:
            assert 'number density' in str(e)

    def test_number_density_units(self):
        builder = MaterialBuilder()
        builder.setName(NAME).setFormula(FORMULA).setNumberDensity(NUMBER_DENSITY)
        builder.setNumberDensityUnit(NumberDensityUnit.FormulaUnits)
        material = builder.build()
        self.assertEqual(material.numberDensity, NUMBER_DENSITY * (2. + 3.))
        self.assertEqual(material.numberDensityEffective, NUMBER_DENSITY * (2. + 3.))
        self.assertEqual(material.packingFraction, 1.)


if __name__ == '__main__':
    unittest.main()
