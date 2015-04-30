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


if __name__ == '__main__':
    unittest.main()