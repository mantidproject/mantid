import unittest
from testhelpers import run_algorithm
from mantid.api import SpectraAxis
import numpy as np

class AxisTest(unittest.TestCase):
  
    _test_ws = None
  
    def setUp(self):
        if self.__class__._test_ws is None:
            alg = run_algorithm("Load", Filename="LOQ48127.raw", SpectrumMax=3, child=True)
            self.__class__._test_ws = alg.getProperty("OutputWorkspace").value
  
    def test_axis_meta_data(self):
        yAxis = self._test_ws.getAxis(1)
        self.assertTrue(isinstance(yAxis, SpectraAxis))
        self.assertTrue(isinstance(yAxis, SpectraAxis))
        self.assertEquals(yAxis.length(), 3)
        self.assertEquals(len(yAxis), yAxis.length())
        self.assertEquals(yAxis.title(), "")

    def test_axis_unit(self):
        xAxis =  self._test_ws.getAxis(0)
        xunit = xAxis.getUnit()
        self.assertEquals(xunit.unitID(), "TOF")
        self.assertEquals(xunit.caption(), "Time-of-flight")
        self.assertEquals(xunit.label(), "microsecond")
        
    def test_value_axis(self):
        yAxis = self._test_ws.getAxis(1)
        for i in range(1,4): # Not including 4
            self.assertEquals(yAxis.getValue(i-1), i)
            
    def test_extract_numerical_axis_values_to_numpy(self):
        yAxis = self._test_ws.getAxis(1)
        values = yAxis.extractValues()
        self.assertTrue(isinstance(values, np.ndarray))
        for index, value in enumerate(values):
            self.assertEquals(value, index + 1)
            
    def test_extract_string_axis_values_to_list(self):
        data = [1.,2.,3.]
        axis_values = ["a","b","c"]
        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=3,VerticalAxisUnit="Text",VerticalAxisValues=axis_values,child=True)
        workspace = alg.getProperty("OutputWorkspace").value
        txtAxis = workspace.getAxis(1)
        self.assertTrue(txtAxis.isText())
        values = txtAxis.extractValues()
        self.assertTrue(isinstance(values, list))
        for index, value in enumerate(values):
            self.assertEquals(value, axis_values[index])
        