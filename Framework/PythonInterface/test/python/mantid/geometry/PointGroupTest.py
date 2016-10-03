# pylint: disable=invalid-name,too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.geometry import PointGroup, PointGroupFactory
from mantid.kernel import V3D


class PointGroupTest(unittest.TestCase):
    def test_creation(self):
        self.assertRaises(ValueError, PointGroupFactory.createPointGroup, "none")

        PointGroupFactory.createPointGroup("m-3m")

    def test_getInfo(self):
        pg = PointGroupFactory.createPointGroup("m-3m")
        self.assertEquals(pg.getName(), "m-3m (Cubic)")
        self.assertEquals(pg.getHMSymbol(), "m-3m")
        self.assertEquals(pg.getCrystalSystem(), PointGroup.CrystalSystem.Cubic)

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

        self.assertEquals(len(equivalents), 2)

    def test_getReflectionFamily(self):
        hkl1 = V3D(0, 0, 1)
        hkl2 = V3D(-1, 0, 0)

        pg = PointGroupFactory.createPointGroup("m-3m")
        self.assertEquals(pg.getReflectionFamily(hkl1), pg.getReflectionFamily(hkl2))

    def test_getLatticeSystem(self):
        pg_rhombohedral = PointGroupFactory.createPointGroup("3m r")
        pg_hexagonal = PointGroupFactory.createPointGroup("3m")

        self.assertEquals(pg_rhombohedral.getLatticeSystem(), PointGroup.LatticeSystem.Rhombohedral)
        self.assertEquals(pg_hexagonal.getLatticeSystem(), PointGroup.LatticeSystem.Hexagonal)


if __name__ == '__main__':
    unittest.main()
