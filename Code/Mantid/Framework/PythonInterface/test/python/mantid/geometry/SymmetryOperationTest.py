import unittest
from mantid.geometry import SymmetryOperation, SymmetryOperationFactoryImpl
from mantid.kernel import V3D

class SymmetryOperationTest(unittest.TestCase):

    def test_creation(self):
        self.assertRaises(RuntimeError, SymmetryOperationFactoryImpl.Instance().createSymOp, "none")

        SymmetryOperationFactoryImpl.Instance().createSymOp("m [001]")

    def test_getInfo(self):
        symOp = SymmetryOperationFactoryImpl.Instance().createSymOp("m [001]")
        self.assertEquals(symOp.order(), 2)
        self.assertEquals(symOp.identifier(), "m [001]")

    def test_apply(self):
        symOp = SymmetryOperationFactoryImpl.Instance().createSymOp("m [001]")

        hkl1 = V3D(1, 1, 1)
        hkl2 = symOp.apply(hkl1)

        self.assertEquals(hkl2, V3D(1, 1, -1))
        self.assertEquals(symOp.apply(hkl2), hkl1)


if __name__ == '__main__':
    unittest.main()