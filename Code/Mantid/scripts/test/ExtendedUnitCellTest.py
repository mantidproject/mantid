import unittest
import math
from mantid.simpleapi import *
from CrystalTools.ExtendedUnitCell import *

def toDegrees(angleInRads):
        return angleInRads*180/math.pi
    
class ExtendedUnitCellTest(unittest.TestCase):
    
    # Miller indices of reflections comprising the horizontal plane.
    recDir1 = [1, 0, 0]
    recDir2 = [0, 0, 1]
    
    decimalPlaces = 6
    
    def test_creation(self):
         a = b = c = 1
         alpha = beta = gamma = 90
         cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
         self.assertTrue(isinstance(cell, ExtendedUnitCell))
         # Check that we can access some sample base methods.
         self.assertEquals(a, cell.a())
         self.assertEquals(b, cell.b())
         self.assertEquals(c, cell.c())
         self.assertEquals(alpha, cell.alpha())
         self.assertEquals(beta, cell.beta())
         self.assertEquals(gamma, cell.gamma())
         
    def test_create_from_existing(self):
        baseCell = UnitCell(1,1,1,90,90,90)
        extendedCell = ExtendedUnitCell(baseCell)
        self.assertTrue(isinstance(extendedCell, ExtendedUnitCell))
         
    def test_inplane_reflections_111(self):
        a = b = c = 1
        alpha = beta = gamma = 90
        cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
        x, y, z = cell.recAnglesFromReflections(self.recDir1, self.recDir2, [1,1,1])
        
        self.assertEquals(toDegrees( math.acos(1/math.sqrt(3))), x)
        self.assertEquals(toDegrees( math.acos(1/math.sqrt(3))), y)
        self.assertEquals(toDegrees( math.acos(1/math.sqrt(3))), z)
     
    def test_inplane_reflections_211(self):
        a = b = c = 1
        alpha = beta = gamma = 90
        cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
        x, y, z = cell.recAnglesFromReflections(self.recDir1, self.recDir2, [2,1,1])
        
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(4)/math.sqrt(6))), x, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(6))), y, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(6))), z, self.decimalPlaces)

    def test_inplane_reflections_311(self):
        a = b = c = 1
        alpha = beta = gamma = 90
        cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
        x, y, z = cell.recAnglesFromReflections(self.recDir1, self.recDir2, [3,1,1])
        
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(9)/math.sqrt(11))), x, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(11))), y, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(11))), z, self.decimalPlaces)

         
    def test_inplane_reflections_311_other_horizontal_plane(self):
        a = b = c = 1
        alpha = beta = gamma = 90
        cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
        
        # Definition of the horizontal plane
        recDir1 = [0,0,1] 
        recDir2 = [0,1,0]
        
        x, y, z = cell.recAnglesFromReflections(recDir1, recDir2, [3,1,1])
        
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(11))), x, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(1)/math.sqrt(11))), y, self.decimalPlaces)
        self.assertAlmostEqual(toDegrees( math.acos(math.sqrt(9)/math.sqrt(11))), z, self.decimalPlaces)
        
    def test_inplane_reflections_111_othorombiccell(self):
        a = b = 1
        c = 2
        alpha = beta = gamma = 90
        cell = createFromLatticeParameters(a, b, c, alpha, beta, gamma)
        
        x, y, z = cell.recAnglesFromReflections(self.recDir1, self.recDir2, [1,1,1])
        
        self.assertTrue(math.acos(math.sqrt(4)/math.sqrt(6)), x)
        self.assertTrue(math.acos(math.sqrt(1)/math.sqrt(6)), y)
        self.assertTrue(math.acos(math.sqrt(1)/math.sqrt(6)), z)

if __name__ == '__main__':
    unittest.main()