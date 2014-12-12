import unittest
from mantid.kernel import DateAndTime
from mantid.geometry import Goniometer
from testhelpers import can_be_instantiated
import numpy as np

class GoniometerTest(unittest.TestCase):

    def setUp(self):
        pass         

    def test_Goniometer_can_be_instantiated(self):
        self.assertTrue(can_be_instantiated(Goniometer))

    def test_getEulerAngles(self):
        g = Goniometer()
        self.assertEquals(g.getEulerAngles()[0], 0)
        self.assertEquals(g.getEulerAngles()[1], 0)
        self.assertEquals(g.getEulerAngles()[2], 0)
        
    def test_getR(self):
        g = Goniometer()
        r = np.array([(1., 0., 0.), (0., 0., 1.), (0., -1., 0.)])
        g.setR(r)
        self.assertTrue( (g.getR() == r).all() )
      

if __name__ == '__main__':
    unittest.main()
