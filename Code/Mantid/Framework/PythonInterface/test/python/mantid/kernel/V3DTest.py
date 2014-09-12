import unittest
import math
from mantid.kernel import V3D

class V3DTest(unittest.TestCase):

    def test_default_construction_is_at_origin(self):
        p = V3D()
        self.assertEquals(p.X(), 0.0)
        self.assertEquals(p.Y(), 0.0)
        self.assertEquals(p.Z(), 0.0)

    def test_construction_with_xyz(self):
        p = V3D(1.5,2.4,6.5)
        self.assertEquals(p.X(), 1.5)
        self.assertEquals(p.Y(), 2.4)
        self.assertEquals(p.Z(), 6.5)

    def test_distance(self):
        a = V3D(0.0,0.0,0.0)
        b = V3D(2.0,2.0,2.0)
        d = a.distance(b)
        self.assertAlmostEquals(d, 2.0*math.sqrt(3.0))

    def test_angle(self):
        a = V3D(2.0, 0.0, 0.0)
        b = V3D(0.0, 1.0, 0.0)
        c = V3D(1.0, 1.0, 0.0)
        d = V3D(-1.0, 0.0, 0.0)
        self.assertAlmostEquals(a.angle(a), 0.0)
        self.assertAlmostEquals(a.angle(b), math.pi / 2.0)
        self.assertAlmostEquals(a.angle(c), math.pi / 4.0)
        self.assertAlmostEquals(a.angle(d), math.pi)

    def test_zenith(self):
        b = V3D(0.0, 0.0, 0.0)
        a = V3D(9.9, 7.6, 0.0)
        self.assertEquals(a.zenith(a), 0.0)
        self.assertAlmostEquals(a.zenith(b), math.pi / 2.0)
        a = V3D(-1.1, 0.0, 0.0);
        self.assertAlmostEquals(a.zenith(b), math.pi / 2.0)
        a = V3D(0.0, 0.0, 1.0);
        self.assertEquals(a.zenith(b), 0.0);
        a = V3D(1.0, 0.0, 1.0);
        self.assertAlmostEquals(a.zenith(b), math.pi / 4.0)
        a = V3D(1.0, 0.0, -1.0);
        self.assertAlmostEquals(a.zenith(b), 3.0 * math.pi / 4.0)

    def test_scalarprod(self):
        a = V3D(1.0,2.0,1.0)
        b = V3D(1.0,-2.0,-1.0)
        sp = a.scalar_prod(b)
        self.assertAlmostEquals(sp,-4.0)

    def test_crossprod(self):
        a = V3D(1.0,0.0,0.0)
        b = V3D(0.0,1.0,0.0)
        c = a.cross_prod(b)
        self.assertAlmostEquals(c.X(),0.0)
        self.assertAlmostEquals(c.Y(),0.0)
        self.assertAlmostEquals(c.Z(),1.0)

    def test_norm(self):
        p = V3D(1.0,-5.0,8.0);
        self.assertAlmostEquals(p.norm(), math.sqrt(90.0))

    def test_norm2(self):
        p = V3D(1.0,-5.0,8.0);
        self.assertAlmostEquals(p.norm2(), 90.0)

    def test_equality_operators_use_value_comparison(self):
        p1 = V3D(1.0,-5.0,8.0)
        p2 = V3D(1.0,-5.0,8.0)
        self.assertTrue(p1 == p2)

    def test_inequality_operators_use_value_comparison(self):
        p1 = V3D(1.0,-5.0,8.0)
        p2 = V3D(1.0,-5.0,8.0) # different objects, same value
        self.assertFalse(p1 != p2)
        p3 = V3D(1.0,-5.0,10.0)
        self.assertTrue(p1 != p3)

    def test_directionAngles_rads(self):
        v = V3D(1, 1, 1)
        inDegrees = False
        angles = v.directionAngles(inDegrees)
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)), angles.X())
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)), angles.Y())
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)), angles.Z())

    def test_directionAngles(self):
        v = V3D(1, 1, 1)
        angles = v.directionAngles()
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)) * 180 / math.pi, angles.X())
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)) * 180 / math.pi, angles.Y())
        self.assertAlmostEquals(math.acos(1.0/math.sqrt(3.0)) * 180 / math.pi, angles.Z())




if __name__ == '__main__':
    unittest.main()
