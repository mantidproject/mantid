import unittest
from mantid.geometry import SymmetryOperation, SymmetryOperationFactoryImpl
from mantid.geometry import SymmetryElement, SymmetryElementFactoryImpl
from mantid.kernel import V3D

class SymmetryElementTest(unittest.TestCase):

    def test_creation_axis(self):
        symOp = SymmetryOperationFactoryImpl.Instance().createSymOp("x,y,-z")
        symEle = SymmetryElementFactoryImpl.Instance().createSymElement(symOp)
	
	self.assertEquals(symEle.hmSymbol(), "m")
	self.assertEquals(symEle.getAxis(), V3D(0,0,1))
	
	rotation = SymmetryOperationFactoryImpl.Instance().createSymOp("x,-y,-z")
        rotationElement = SymmetryElementFactoryImpl.Instance().createSymElement(rotation)
	
	self.assertEquals(rotationElement.hmSymbol(), "2")
	self.assertEquals(rotationElement.getAxis(), V3D(1,0,0))

    def test_creation_no_axis(self):
        symOp = SymmetryOperationFactoryImpl.Instance().createSymOp("-x,-y,-z")
        symEle = SymmetryElementFactoryImpl.Instance().createSymElement(symOp)
	
	self.assertEquals(symEle.hmSymbol(), "-1")
	self.assertEquals(symEle.getAxis(), V3D(0,0,0))


if __name__ == '__main__':
    unittest.main()