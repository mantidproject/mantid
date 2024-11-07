# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.geometry import SymmetryOperationFactory
from mantid.kernel import V3D


class SymmetryOperationTest(unittest.TestCase):
    def test_creation(self):
        self.assertRaises(RuntimeError, SymmetryOperationFactory.createSymOp, "none")

        SymmetryOperationFactory.createSymOp("x,y,-z")

    def test_getInfo(self):
        symOp = SymmetryOperationFactory.createSymOp("x, y, -z")
        self.assertEqual(symOp.getOrder(), 2)
        self.assertEqual(symOp.getIdentifier(), "x,y,-z")

    def test_apply(self):
        symOp = SymmetryOperationFactory.createSymOp("x,y,-z")

        hkl1 = V3D(1, 1, 1)
        hkl2 = symOp.apply(hkl1)

        self.assertEqual(hkl2, V3D(1, 1, -1))
        self.assertEqual(symOp.transformHKL(hkl2), hkl1)


if __name__ == "__main__":
    unittest.main()
