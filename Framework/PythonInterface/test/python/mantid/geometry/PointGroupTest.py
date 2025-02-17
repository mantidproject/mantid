# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods
import unittest
from mantid.geometry import PointGroup, PointGroupFactory
from mantid.kernel import V3D


class PointGroupTest(unittest.TestCase):
    def test_creation(self):
        self.assertRaises(ValueError, PointGroupFactory.createPointGroup, "none")

        PointGroupFactory.createPointGroup("m-3m")

    def test_getInfo(self):
        pg = PointGroupFactory.createPointGroup("m-3m")
        self.assertEqual(pg.getName(), "m-3m (Cubic)")
        self.assertEqual(pg.getHMSymbol(), "m-3m")
        self.assertEqual(pg.getCrystalSystem(), PointGroup.CrystalSystem.Cubic)

    def test_isEquivalent(self):
        hkl1 = V3D(1, 1, 1)
        hkl2 = V3D(-1, -1, -1)
        hkl3 = V3D(-1, -1, 2)

        pg = PointGroupFactory.createPointGroup("m-3m")
        self.assertTrue(pg.isEquivalent(hkl1, hkl2))
        self.assertFalse(pg.isEquivalent(hkl1, hkl3))

    def test_getEquivalents(self):
        hkl1 = V3D(1, 0, 0)
        hkl2 = V3D(-1, 0, 0)

        pg = PointGroupFactory.createPointGroup("-1")
        equivalents = pg.getEquivalents(hkl1)

        self.assertTrue(hkl1 in equivalents)
        self.assertTrue(hkl2 in equivalents)

        self.assertEqual(len(equivalents), 2)

    def test_getReflectionFamily(self):
        hkl1 = V3D(0, 0, 1)
        hkl2 = V3D(-1, 0, 0)

        pg = PointGroupFactory.createPointGroup("m-3m")
        self.assertEqual(pg.getReflectionFamily(hkl1), pg.getReflectionFamily(hkl2))

    def test_getLatticeSystem(self):
        pg_rhombohedral = PointGroupFactory.createPointGroup("3m r")
        pg_hexagonal = PointGroupFactory.createPointGroup("3m")

        self.assertEqual(pg_rhombohedral.getLatticeSystem(), PointGroup.LatticeSystem.Rhombohedral)
        self.assertEqual(pg_hexagonal.getLatticeSystem(), PointGroup.LatticeSystem.Hexagonal)

    def test_to_string(self):
        pg = PointGroupFactory.createPointGroup("m-3m")

        expected_str = "Point group with:\nLattice system: Cubic\nCrystal system: Cubic\nSymbol: m-3m"

        expected_repr = 'PointGroupFactory.createPointGroup("m-3m")'

        self.assertEqual(expected_str, str(pg))
        self.assertEqual(expected_repr, pg.__repr__())

        newPg = eval(pg.__repr__())
        self.assertEqual(pg.getHMSymbol(), newPg.getHMSymbol())


if __name__ == "__main__":
    unittest.main()
