# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-public-methods
import unittest
from mantid.geometry import CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from mantid.kernel import V3D
import numpy as np


class CrystalStructureTest(unittest.TestCase):
    crystalStructure = CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.02")

    def test_create(self):
        _ = ReflectionGenerator(self.crystalStructure)
        _ = ReflectionGenerator(self.crystalStructure, ReflectionConditionFilter.Centering)

    def test_getHKLs(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = generator.getHKLs(1.0, 10.0)

        self.assertEqual(len(hkls), 138)

        # default is filtering by space group reflection condition
        self.assertTrue(V3D(2, 2, 2) in hkls)
        self.assertFalse(V3D(1, 0, 0) in hkls)

    def test_getHKLsUsingFilter(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = generator.getHKLsUsingFilter(1.0, 10.0, ReflectionConditionFilter.StructureFactor)

        self.assertEqual(len(hkls), 130)

        # The 222 is gone now
        self.assertFalse(V3D(2, 2, 2) in hkls)

    def test_getUniqueHKLs(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = generator.getUniqueHKLs(1.0, 10.0)

        self.assertEqual(len(hkls), 9)

        self.assertTrue(V3D(2, 2, 2) in hkls)
        self.assertFalse(V3D(1, 0, 0) in hkls)

    def test_getUniqueHKLsUsingFilter(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = generator.getUniqueHKLsUsingFilter(1.0, 10.0, ReflectionConditionFilter.StructureFactor)

        self.assertEqual(len(hkls), 8)
        self.assertFalse(V3D(2, 2, 2) in hkls)

    def test_getDValues(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = [V3D(1, 0, 0), V3D(1, 1, 1)]

        dValues = generator.getDValues(hkls)

        self.assertEqual(len(hkls), len(dValues))
        self.assertAlmostEqual(dValues[0], 5.431, places=10)
        self.assertAlmostEqual(dValues[1], 5.431 / np.sqrt(3.0), places=10)

    def test_getFsSquared(self):
        generator = ReflectionGenerator(self.crystalStructure)
        hkls = generator.getUniqueHKLs(1.0, 10.0)

        fsSquared = generator.getFsSquared(hkls)

        self.assertEqual(len(fsSquared), len(hkls))


if __name__ == "__main__":
    unittest.main()
