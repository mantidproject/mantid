# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.geometry import SymmetryOperation, SymmetryOperationFactory
from mantid.geometry import SymmetryElement, SymmetryElementFactory
from mantid.kernel import V3D

class SymmetryElementTest(unittest.TestCase):

    def test_creation_axis(self):
        symOp = SymmetryOperationFactory.createSymOp("x,y,-z")
        symEle = SymmetryElementFactory.createSymElement(symOp)

        self.assertEquals(symEle.getHMSymbol(), "m")
        self.assertEquals(symEle.getAxis(), V3D(0,0,1))

        rotation = SymmetryOperationFactory.createSymOp("x,-y,-z")
        rotationElement = SymmetryElementFactory.createSymElement(rotation)

        self.assertEquals(rotationElement.getHMSymbol(), "2")
        self.assertEquals(rotationElement.getAxis(), V3D(1,0,0))

    def test_creation_no_axis(self):
        symOp = SymmetryOperationFactory.createSymOp("-x,-y,-z")
        symEle = SymmetryElementFactory.createSymElement(symOp)

        self.assertEquals(symEle.getHMSymbol(), "-1")
        self.assertEquals(symEle.getAxis(), V3D(0,0,0))

    def test_creation_rotation(self):
        symOpPlus = SymmetryOperationFactory.createSymOp("-y,x-y,z")
        symElePlus = SymmetryElementFactory.createSymElement(symOpPlus)

        self.assertEquals(symElePlus.getRotationSense(), SymmetryElement.RotationSense.Positive)

        symOpMinus = SymmetryOperationFactory.createSymOp("-x+y,-x,z")
        symEleMinus = SymmetryElementFactory.createSymElement(symOpMinus)

        self.assertEquals(symEleMinus.getRotationSense(), SymmetryElement.RotationSense.Negative)

    def test_creation_no_rotation(self):
        symOpNone = SymmetryOperationFactory.createSymOp("-x,-y,-z")
        symEleNone = SymmetryElementFactory.createSymElement(symOpNone)

        self.assertEquals(symEleNone.getRotationSense(), SymmetryElement.RotationSense.NoRotation)


if __name__ == '__main__':
    unittest.main()
