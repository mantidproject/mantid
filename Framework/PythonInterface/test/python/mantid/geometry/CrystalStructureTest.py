# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-public-methods,broad-except
import unittest
from mantid.geometry import CrystalStructure


class CrystalStructureTest(unittest.TestCase):
    def test_creation(self):
        # Some valid constructions
        self.assertTrue(self.createCrystalStructureOrRaise("5.43 5.43 5.43", "F d -3 m", "Al 1/3 0.454 1/12 1.0 0.01"))
        self.assertTrue(self.createCrystalStructureOrRaise("5.43 5.43 5.43", "C m m m", "Al 1/3 0.454 1/12 1.0 0.01;\nSi 2/3 0.121 1/8"))
        self.assertTrue(
            self.createCrystalStructureOrRaise("5.43 5.43 5.43 90 90 120", "R -3 c", "Al 1/3 0.454 1/12 1.0 0.01;\nSi 2/3 0.121 1/8")
        )

        # Invalid unit cell specification
        self.assertFalse(self.createCrystalStructureOrRaise("5.43 5.43 5.43 90.0", "C m m m", "Al 1/3 0.454 1/12 1.0 0.01"))

        # Invalid space group
        self.assertFalse(self.createCrystalStructureOrRaise("5.43 5.43 5.43", "INVALID", "Al 1/3 0.454 1/12 1.0 0.01"))

        # Invalid atom specification
        self.assertFalse(self.createCrystalStructureOrRaise("5.43 5.43 5.43", "C m c e", "Al 1/3 0"))

    def createCrystalStructureOrRaise(self, unitCell, spaceGroup, atomStrings):
        try:
            CrystalStructure(unitCell, spaceGroup, atomStrings)
            return True
        except Exception:
            return False

    def test_UnitCell(self):
        structure = CrystalStructure("5.43 5.42 5.41", "F d -3 m", "Al 1/3 0.454 1/12 1.0 0.01")
        cell = structure.getUnitCell()

        self.assertEqual(cell.a(), 5.43)
        self.assertEqual(cell.b(), 5.42)
        self.assertEqual(cell.c(), 5.41)

    def test_SpaceGroup(self):
        structure = CrystalStructure("5.43 5.42 5.41", "F d -3 m", "Al 1/3 0.454 1/12 1.0 0.01")
        spaceGroup = structure.getSpaceGroup()

        self.assertEqual(spaceGroup.getHMSymbol(), "F d -3 m")

    def test_scatterers(self):
        initialString = "Al 1/3 0.454 1/12 1 0.01;Si 0.1 0.2 0.3 0.99 0.1"

        structure = CrystalStructure("5.43 5.42 5.41", "F d -3 m", initialString)
        scatterers = structure.getScatterers()

        self.assertEqual(";".join(scatterers), initialString)

    def test_to_string(self):
        initialString = "Al 1/3 0.454 1/12 1 0.01;Si 0.1 0.2 0.3 0.99 0.1"
        structure = CrystalStructure("5.43 5.42 5.41", "F d -3 m", initialString)

        expected_str = (
            "Crystal structure with:\nUnit cell: a = 5.43 b = 5.42 "
            "c = 5.41 alpha = 90 beta = 90 gamma = 90\n"
            "Centering: All-face centred\nSpace Group: F d -3 m\n"
            "Scatterers: Al 1/3 0.454 1/12 1 0.01, "
            "Si 0.1 0.2 0.3 0.99 0.1"
        )

        expected_repr = 'CrystalStructure("5.43 5.42 5.41 90 90 90", "F d -3 m", "Al 1/3 0.454 1/12 1 0.01; Si 0.1 0.2 0.3 0.99 0.1")'

        self.assertEqual(expected_str, str(structure))
        self.assertEqual(expected_repr, structure.__repr__())

        newStructure = eval(structure.__repr__())
        self.assertEqual(structure.getUnitCell().a(), newStructure.getUnitCell().a())


if __name__ == "__main__":
    unittest.main()
