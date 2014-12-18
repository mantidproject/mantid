import unittest
from mantid.geometry import SpaceGroup, SpaceGroupFactoryImpl

class SpaceGroupTest(unittest.TestCase):

    def test_creation(self):
        self.assertRaises(ValueError, SpaceGroupFactoryImpl.Instance().createSpaceGroup, "none")

        SpaceGroupFactoryImpl.Instance().createSpaceGroup("I m -3 m")

    def test_interface(self):
        spaceGroup = SpaceGroupFactoryImpl.Instance().createSpaceGroup("P -1")
        self.assertEquals(spaceGroup.hmSymbol(), "P -1")
        self.assertEquals(spaceGroup.order(), 2)

        symOpStrings = spaceGroup.getSymmetryOperationStrings()

        self.assertEqual(len(symOpStrings), 2)
        self.assertTrue("x,y,z" in symOpStrings)
        self.assertTrue("-x,-y,-z" in symOpStrings)

    def test_equivalentPositions(self):
        spaceGroup = SpaceGroupFactoryImpl.Instance().createSpaceGroup("P -1")

        position = [0.34, 0.3, 0.4]
        equivalentPositions = spaceGroup.getEquivalentPositions(position)

        self.assertEqual(len(equivalentPositions), 2)

if __name__ == '__main__':
    unittest.main()