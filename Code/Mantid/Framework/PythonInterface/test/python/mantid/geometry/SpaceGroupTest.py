#pylint: disable=no-init,invalid-name,too-many-public-methods
import unittest
from mantid.geometry import SpaceGroupFactory


class SpaceGroupTest(unittest.TestCase):
    def test_creation(self):
        self.assertRaises(ValueError, SpaceGroupFactory.createSpaceGroup, "none")

        SpaceGroupFactory.createSpaceGroup("I m -3 m")

    def test_interface(self):
        spaceGroup = SpaceGroupFactory.createSpaceGroup("P -1")
        self.assertEquals(spaceGroup.getHMSymbol(), "P -1")
        self.assertEquals(spaceGroup.getOrder(), 2)

        symOpStrings = spaceGroup.getSymmetryOperationStrings()

        self.assertEqual(len(symOpStrings), 2)
        self.assertTrue("x,y,z" in symOpStrings)
        self.assertTrue("-x,-y,-z" in symOpStrings)

    def test_equivalentPositions_Triclinic(self):
        wyckoffs = [([0.3, 0.4, 0.45], 2),
                    ([0.5, 0.5, 0.5], 1),
                    ([0.0, 0.5, 0.5], 1),
                    ([0.5, 0.0, 0.5], 1),
                    ([0.5, 0.5, 0.0], 1),
                    ([0.5, 0.0, 0.0], 1),
                    ([0.0, 0.5, 0.0], 1),
                    ([0.0, 0.0, 0.5], 1),
                    ([0.0, 0.0, 0.0], 1)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("P -1")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Monoclinic(self):
        wyckoffs = [([0.3, 0.4, 0.45], 8),
                    ([0.0, 0.4, 0.25], 4),
                    ([0.25, 0.25, 0.5], 4),
                    ([0.25, 0.25, 0.0], 4),
                    ([0.0, 0.5, 0.0], 4),
                    ([0.0, 0.0, 0.0], 4)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("C 1 2/c 1")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Orthorhombic(self):
        wyckoffs = [([0.3, 0.4, 0.45], 16),
                    ([0.3, 0.25, 0.45], 8),
                    ([0.0, 0.4, 0.45], 8),
                    ([0.25, 0.4, 0.25], 8),
                    ([0.3, 0.0, 0.0], 8),
                    ([0.0, 0.25, 0.45], 4),
                    ([0.25, 0.25, 0.75], 4),
                    ([0.25, 0.25, 0.25], 4),
                    ([0.0, 0.0, 0.5], 4),
                    ([0.0, 0.0, 0.0], 4)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("I m m a")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Tetragonal(self):
        wyckoffs = [([0.3, 0.4, 0.45], 32),
                    ([0.3, 0.3, 0.25], 16),
                    ([0.25, 0.4, 0.125], 16),
                    ([0.0, 0.0, 0.45], 16),
                    ([0.0, 0.25, 0.125], 16),
                    ([0.0, 0.0, 0.25], 8),
                    ([0.0, 0.0, 0.0], 8)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("I 41/a c d")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Trigonal(self):
        wyckoffs = [([0.3, 0.4, 0.45], 36),
                    ([0.3, 0.0, 0.25], 18),
                    ([0.5, 0.0, 0.0], 18),
                    ([0.0, 0.0, 0.45], 12),
                    ([0.0, 0.0, 0.0], 6),
                    ([0.0, 0.0, 0.25], 6)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("R -3 c")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Hexagonal(self):
        wyckoffs = [([0.3, 0.4, 0.45], 24),
                    ([0.3, 0.6, 0.45], 12),
                    ([0.3, 0.4, 0.25], 12),
                    ([0.3, 0.0, 0.0], 12),
                    ([0.3, 0.6, 0.25], 6),
                    ([0.5, 0.0, 0.0], 6),
                    ([1. / 3., 2. / 3., 0.45], 4),
                    ([0.0, 0.0, 0.45], 4),
                    ([1. / 3, 2. / 3., 0.75], 2),
                    ([1. / 3, 2. / 3., 0.25], 2),
                    ([0.0, 0.0, 0.25], 2),
                    ([0.0, 0.0, 0.0], 2)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("P 63/m m c")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def test_equivalentPositions_Cubic(self):
        wyckoffs = [([0.3, 0.4, 0.45], 96),
                    ([0.3, 0.25, 0.25], 48),
                    ([0.3, 0.0, 0.0], 48),
                    ([0.3, 0.3, 0.3], 32),
                    ([0.25, 0.0, 0.0], 24),
                    ([0.0, 0.25, 0.25], 24),
                    ([0.25, 0.25, 0.25], 8),
                    ([0.0, 0.0, 0.0], 8)]

        spaceGroup = SpaceGroupFactory.createSpaceGroup("F -4 3 c")
        self.checkWyckoffPositions(spaceGroup, wyckoffs)

    def checkWyckoffPositions(self, spaceGroup, wyckoffs):
        for wp in wyckoffs:
            equivalentPositions = spaceGroup.getEquivalentPositions(wp[0])
            self.assertEqual(len(equivalentPositions), wp[1])


if __name__ == '__main__':
    unittest.main()
