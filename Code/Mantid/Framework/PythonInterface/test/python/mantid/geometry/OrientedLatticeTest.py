import unittest
import testhelpers
from mantid.geometry import OrientedLattice, UnitCell
from mantid.kernel import V3D
import numpy as np

class OrientedLatticeTest(unittest.TestCase):

    def test_OrientedLattice_is_subclass_of_UnitCell(self):
        self.assertTrue(issubclass(OrientedLattice, UnitCell))

    def test_simple_values(self):
        u1 = OrientedLattice()
        self.assertEquals(u1.a1(),1)
        self.assertEquals(u1.alpha(),90)

        u2 = OrientedLattice(3,4,5)
        self.assertAlmostEqual(u2.b1(),1./3.,10)
        self.assertAlmostEqual(u2.alphastar(),90,10)
        u3 = u2;
        self.assertAlmostEqual(u3.volume(),1./u2.recVolume(),10)
        u2.seta(3);
        self.assertAlmostEqual(u2.a(),3,10)

    def test_setu_matrix_from_vectors(self):
        def run_test(v1, v2):
            cell = OrientedLattice()
            testhelpers.assertRaisesNothing(self, cell.setUFromVectors, v1, v2)
            rot = cell.getUB();
            expected = np.array([(0,1.,0.), (0.,0.,1.), (1.,0.,0.)])
            np.testing.assert_array_almost_equal(expected, rot, 8)

        # Set from V3Ds
        run_test(V3D(1,0,0),V3D(0,1,0))
        # Set from Python lists
        run_test([1,0,0],[0,1,0])
        # Set from numpy arrays
        run_test(np.array([1,0,0]),np.array([0,1,0]))

    def test_qFromHKL(self):
        ol = OrientedLattice(1,1,1)
        hkl = V3D(1,1,1)
        qVec = ol.qFromHKL(hkl)
        self.assertAlmostEqual(hkl.X() * 2 * np.pi, qVec.X(), 9)
        self.assertAlmostEqual(hkl.Y() * 2 * np.pi, qVec.Y(), 9)
        self.assertAlmostEqual(hkl.Z() * 2 * np.pi, qVec.Z(), 9)


    def test_hklFromQ(self):
        ol = OrientedLattice(1,1,1)
        qVec = V3D(1,1,1)
        hkl = ol.hklFromQ(qVec)
        self.assertAlmostEqual(hkl.X() * 2 * np.pi, qVec.X(), 9)
        self.assertAlmostEqual(hkl.Y() * 2 * np.pi, qVec.Y(), 9)
        self.assertAlmostEqual(hkl.Z() * 2 * np.pi, qVec.Z(), 9)

    def test_qFromHLK_input_types(self):
        '''
        Test that you can provide input hkl values as different python types.
        '''
        ol = OrientedLattice(1,1,1)

        self.assertTrue(isinstance( ol.qFromHKL(V3D(1,1,1)), V3D), "Should work with V3D input")

        self.assertTrue(isinstance( ol.qFromHKL([1,1,1]), V3D), "Should work with python array input" )

        self.assertTrue(isinstance( ol.qFromHKL(np.array([1,1,1])), V3D), "Should work with numpy array input" )

    def test_hklFromQ_input_types(self):
        '''
        Test that you can provide input q values as different python types.
        '''
        ol = OrientedLattice(1,1,1)

        self.assertTrue(isinstance( ol.hklFromQ(V3D(1,1,1)), V3D), "Should work with V3D input")

        self.assertTrue(isinstance( ol.hklFromQ([1,1,1]), V3D), "Should work with python array input" )

        self.assertTrue(isinstance( ol.hklFromQ(np.array([1,1,1])), V3D), "Should work with numpy array input" )

if __name__ == '__main__':
    unittest.main()