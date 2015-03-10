import unittest
from mantid.api import Projection
from mantid.kernel import VMD

class ProjectionTest(unittest.TestCase):

    def test_constructors(self):
        self.assertEqual(Projection().getNumDims(), 2)

        self.assertRaises(ValueError, Projection, 0)
        self.assertRaises(ValueError, Projection, 1)

        self.assertEqual(Projection(2).getNumDims(), 2)
        self.assertEqual(Projection(3).getNumDims(), 3)
        self.assertEqual(Projection(10).getNumDims(), 10)

        p = Projection(VMD(0,1),VMD(1,2))
        self.assertEqual(p.getNumDims(), 3)

        p = Projection(VMD(0,1,2),
                       VMD(3,4,5),
                       VMD(6,7,8))
        self.assertEqual(p.getNumDims(), 3)
        self.assertEqual(p.getAxis(0), VMD(0,1,2))

        p = Projection(VMD( 0, 1, 2, 3),
                       VMD( 4, 5, 6, 7),
                       VMD( 8, 9,10,11),
                       VMD(12,13,14,15))
        self.assertEqual(p.getNumDims(), 4)
        self.assertEqual(p.getAxis(1), VMD(4,5,6,7))

        p = Projection(VMD( 0, 1, 2, 3, 4),
                       VMD( 5, 6, 7, 8, 9),
                       VMD(10,11,12,13,14),
                       VMD(14,15,16,17,18),
                       VMD(19,20,21,22,23))
        self.assertEqual(p.getNumDims(), 5)
        self.assertEqual(p.getAxis(4), VMD(19,20,21,22,23))

        p = Projection(VMD( 0, 1, 2, 3, 4, 5),
                       VMD( 6, 7, 8, 9,10,11),
                       VMD(12,13,14,15,16,17),
                       VMD(18,19,20,21,22,23),
                       VMD(24,25,26,27,28,29),
                       VMD(30,31,32,33,34,35))
        self.assertEqual(p.getNumDims(), 6)
        self.assertEqual(p.getAxis(5), VMD(30,31,32,33,34,35))

    def test_accessors(self):
        p = Projection(3)
        self.assertEqual(p.getNumDims(), 3)

        p.setAxis(0, VMD(0,1,2))
        p.setAxis(1, VMD(3,4,5))
        p.setAxis(2, VMD(6,7,8))
        self.assertEqual(p.getAxis(0), VMD(0,1,2))
        self.assertEqual(p.getAxis(1), VMD(3,4,5))
        self.assertEqual(p.getAxis(2), VMD(6,7,8))

        p.setOffset(0, 1)
        p.setOffset(1, 4)
        p.setOffset(2, 9)
        self.assertEqual(p.getOffset(0), 1)
        self.assertEqual(p.getOffset(1), 4)
        self.assertEqual(p.getOffset(2), 9)

        p.setType(0, 'r')
        p.setType(1, 'a')
        p.setType(2, 'r')
        self.assertEqual(p.getType(0), 'r')
        self.assertEqual(p.getType(1), 'a')
        self.assertEqual(p.getType(2), 'r')

    def test_uvw(self):
        p = Projection(VMD(0,1,2),
                       VMD(3,4,5),
                       VMD(6,7,8))

        self.assertEqual(p.u, VMD(0,1,2))
        self.assertEqual(p.v, VMD(3,4,5))
        self.assertEqual(p.w, VMD(6,7,8))

        p.u[1] = 5
        p.v = VMD(7,8,9)
        p.w[2] = 0

        self.assertEqual(p.u, VMD(0,5,2))
        self.assertEqual(p.v, VMD(7,8,9))
        self.assertEqual(p.w, VMD(6,7,0))

if __name__ == '__main__':
    unittest.main()
