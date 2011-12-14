import unittest
from mantid import UnitCell
import numpy as np

class UnitCellTest(unittest.TestCase):
  
    def test_invalid_parameters_throw(self):
        self.assertRaises(RuntimeError, UnitCell, 0, 0, 0, 0, 0, 0)
        
    def test_simple_constructor(self):
        u1 = UnitCell()
        self.assertEquals(u1.a1(), 1)
        self.assertEquals(u1.alpha(), 90)
        
        u2 = UnitCell(3,4,5)
        self.assertAlmostEquals(u2.b1(),1./3., 10)
        self.assertAlmostEquals(u2.alphastar(), 90, 10)
        
        u4 = u2
        self.assertAlmostEquals(u4.volume(),1./u2.recVolume(),10)
        u2.seta(3);
        self.assertAlmostEquals(u2.a(),3,10)
        
    def test_numpy_array_conversion(self):
        row0 = (0.162546756312, 0.00815256992072, -0.00145274558861)
        row1 = (row0[1], 0.028262965555, 0.00102046431298)
        row2 = (row0[2], row1[2], 0.0156808990098 )
        gstar = np.array( [row0,row1,row2] )
        
        u = UnitCell()
        try:
            u.recalculateFromGstar(gstar)
        except Exception:
            self.fail("recalculateFromGstar should not raise an exception")
        
        self._check_cell(u)

    def _check_cell(self, cell):
        self.assertAlmostEqual(cell.a(),2.5,10)
        self.assertAlmostEqual(cell.b(),6,10)
        self.assertAlmostEqual(cell.c(),8,10)
        self.assertAlmostEqual(cell.alpha(),93,10)
        self.assertAlmostEqual(cell.beta(),88,10)
        self.assertAlmostEqual(cell.gamma(),97,10)
        
        # get the some elements of the B matrix
#        self.assertAlmostEqual(cell.getB()[0][0],0.403170877311,10)
#        self.assertAlmostEqual(cell.getB()[2][0],0.0,10)
#        self.assertAlmostEqual(cell.getB()[0][2],-0.00360329991666,10)
#        self.assertAlmostEqual(cell.getB()[2][2],0.125,10)

#
#  void checkCell(UnitCell & u)
#  {
#
#
#    // Inverse B matrix
#    DblMatrix I = u.getB() * u.getBinv();
#    TS_ASSERT_EQUALS( I, Matrix<double>(3,3,true));
#
#    // d spacing for direct lattice at (1,1,1) (will automatically check dstar)
#    TS_ASSERT_DELTA(u.d(1.,1.,1.),2.1227107587,1e-10);
#    TS_ASSERT_DELTA(u.d(V3D(1.,1.,1.)),2.1227107587,1e-10);
#    // angle
#    TS_ASSERT_DELTA(u.recAngle(1,1,1,1,0,0,angRadians),0.471054990614,1e-10);
#  }
#
#  void test_Advanced()
#  {
#    // test more advanced calculations
#    // the new Gstar shold yield a=2.5, b=6, c=8, alpha=93, beta=88, gamma=97.
#    DblMatrix newGstar(3,3);
#    newGstar[0][0]=0.162546756312;
#    newGstar[0][1]=0.00815256992072;
#    newGstar[0][2]=-0.00145274558861;
#    newGstar[1][0]=newGstar[0][1];
#    newGstar[1][1]=0.028262965555;
#    newGstar[1][2]=0.00102046431298;
#    newGstar[2][0]=newGstar[0][2];
#    newGstar[2][1]=newGstar[1][2];
#    newGstar[2][2]=0.0156808990098;
#
#    UnitCell u;
#    u.recalculateFromGstar(newGstar);
#
#    // Check the directly-created one
#    checkCell(u);
#
#    // Check if copy constructor is also good.
#    UnitCell u2 = u;
#    checkCell(u2);
#
#  }