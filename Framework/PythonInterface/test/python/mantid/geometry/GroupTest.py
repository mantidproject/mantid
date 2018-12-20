# pylint: disable=no-init,invalid-name,too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.geometry import Group, SpaceGroupFactory, PointGroupFactory, UnitCell


class GroupTest(unittest.TestCase):
    def test_creationFromString(self):
        group = Group('x,y,z')
        self.assertEqual(group.getOrder(), 1)

        self.assertRaises(RuntimeError, Group, 'invalid')

    def test_creationFromVector(self):
        spaceGroup = SpaceGroupFactory.createSpaceGroup("P 63/m m c")
        symOps = spaceGroup.getSymmetryOperations()

        group = Group(symOps)
        self.assertEqual(group.getOrder(), spaceGroup.getOrder())

    def test_creationFromPythonList(self):
        spaceGroup = SpaceGroupFactory.createSpaceGroup("P 63/m m c")

        # Construct python list of only certain symmetry operations
        symOps = [x for x in spaceGroup.getSymmetryOperations() if x.getOrder() == 6]

        group = Group(symOps)
        self.assertEqual(group.getOrder(), len(symOps))

        # But the constructed group is not actually a group
        self.assertFalse(group.isGroup())

    def test_isInvariant_tolerance(self):
        pg3barHex = PointGroupFactory.createPointGroup('-3')
        pg3barRh = PointGroupFactory.createPointGroup('-3 r')

        cellHex = UnitCell(3, 3, 6, 90, 90, 120)
        cellRh = UnitCell(3, 3, 3, 75, 75, 75)

        self.assertTrue(pg3barHex.isInvariant(cellHex.getG()))
        self.assertFalse(pg3barHex.isInvariant(cellRh.getG()))

        self.assertTrue(pg3barRh.isInvariant(cellRh.getG()))
        self.assertFalse(pg3barRh.isInvariant(cellHex.getG()))


if __name__ == '__main__':
    unittest.main()
