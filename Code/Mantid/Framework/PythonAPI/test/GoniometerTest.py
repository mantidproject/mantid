import unittest

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

class GoniometerTest(unittest.TestCase):
    """
    Test the interface to Goniometers
    """
    
    def setUp(self):
        mtd.clear()
        pass
    
    def tearDown(self):
        pass
         
    def test_get_default_axis_number(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()
        self.assertEquals(0, goniometer.getNumberAxes())

    def test_push_several_axis(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()
        goniometer.pushAxis("Axis1", axisX=1.0, axisY=0.0, axisZ=0.0, angle=30.0, sense=CW)
        goniometer.pushAxis("Axis2", 1.0, 0.0, 0.0,20.0)
        self.assertEquals(2, goniometer.getNumberAxes())	

    def test_get_axis(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()     
        goniometer.pushAxis("Axis1", axisX=1.0, axisY=0.0, axisZ=0.0, angle=4.0, sense=CW)
        axis = goniometer.getAxis(name="Axis1")
        
        self.assertEquals(axis.name, "Axis1")
        self.assertEquals(4.0, axis.angle)
        self.assertEquals(CW, axis.sense)
        self.assertEquals(-1, axis.sense)
     
    def test_set_rotation_angle_by_name(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()     
        goniometer.pushAxis("Axis1", axisX=1.0, axisY=0.0, axisZ=0.0, angle=4.0)
        
        goniometer.setRotationAngle(name="Axis1", angle=10)
        axis = goniometer.getAxis(name="Axis1")
        
        self.assertEquals(10, axis.angle)

    def test_set_rotation_angle_by_angle(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()     
        goniometer.pushAxis("Axis1", axisX=1.0, axisY=0.0, axisZ=0.0, angle=4.0)
        
        goniometer.setRotationAngle(axisNumber=0, angle=10)
        axis = goniometer.getAxis(name="Axis1")
        
        self.assertEquals(10, axis.angle)
        
    def test_make_universal_goniometer(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()  
        goniometer.makeUniversalGoniometer()
        self.assertEquals(3, goniometer.getNumberAxes())
        
    def test_get_Euler_angles(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()     
        angles = goniometer.getEulerAngles()
        self.assertEquals(3, len(angles))
     
    def test_axes_info(self):
        ws = LoadRaw('LOQ48127.raw', 'ws', SpectrumMin=1, SpectrumMax=1).workspace()
        goniometer = ws.getRun().getGoniometer()
        goniometer.pushAxis("Axis1", axisX=1.0, axisY=0.0, axisZ=0.0, angle=30.0, sense=CW)
        info = goniometer.axesInfo()
        self.assertTrue(len(info) > 0)
if __name__ == '__main__':
    unittest.main()

    
