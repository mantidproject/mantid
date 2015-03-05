import unittest
from mantid.geometry import SymmetryOperation, SymmetryOperationFactory
from mantid.kernel import V3D

class SymmetryOperationTest(unittest.TestCase):

    def test_creation(self):
        self.assertRaises(RuntimeError, SymmetryOperationFactory.createSymOp, "none")

        SymmetryOperationFactory.createSymOp("x,y,-z")

    def test_getInfo(self):
        symOp = SymmetryOperationFactory.createSymOp("x, y, -z")
        self.assertEquals(symOp.order(), 2)
        self.assertEquals(symOp.identifier(), "x,y,-z")

    def test_apply(self):
        symOp = SymmetryOperationFactory.createSymOp("x,y,-z")

        hkl1 = V3D(1, 1, 1)
        hkl2 = symOp.apply(hkl1)

        self.assertEquals(hkl2, V3D(1, 1, -1))
        self.assertEquals(symOp.apply(hkl2), hkl1)


if __name__ == '__main__':
    unittest.main()