#pylint: disable=no-init,invalid-name,too-many-public-methods
import unittest
from mantid.geometry import Group, SpaceGroupFactory

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


if __name__ == '__main__':
    unittest.main()
