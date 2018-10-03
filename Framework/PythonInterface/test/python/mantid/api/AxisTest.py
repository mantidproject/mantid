# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import run_algorithm
from mantid.api import NumericAxis, SpectraAxis, TextAxis, mtd
import numpy as np

class AxisTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self.__class__._test_ws is None:
                    datY=[1,2,3]
                    alg = run_algorithm('CreateWorkspace', DataX=datY,DataY=datY,DataE=datY,NSpec=3,
                                UnitX='TOF',child=True)
                    self.__class__._test_ws = alg.getProperty("OutputWorkspace").value

    def test_constructor_methods_return_the_correct_type(self):
        self.assertTrue(isinstance(NumericAxis.create(2), NumericAxis))
        self.assertTrue(isinstance(SpectraAxis.create(self._test_ws), SpectraAxis))
        self.assertTrue(isinstance(TextAxis.create(2), TextAxis))

    def test_axis_meta_data(self):
        yAxis = self._test_ws.getAxis(1)
        self.assertEquals(yAxis.length(), 3)
        self.assertEquals(len(yAxis), yAxis.length())
        self.assertEquals(yAxis.title(), "")

    def test_axis_unit(self):
        xAxis =  self._test_ws.getAxis(0)
        xunit = xAxis.getUnit()
        self.assertEquals(xunit.unitID(), "TOF")
        self.assertEquals(xunit.caption(), "Time-of-flight")
        self.assertEquals(xunit.label(), "microsecond")

    def test_axis_unit_can_be_replaced(self):
        datY=[1,2,3]
        ns=3
        taxis = [1,2,3]
        alg = run_algorithm('CreateWorkspace', DataX=datY,DataY=datY,DataE=datY,NSpec=ns,
                            VerticalAxisUnit="Label", VerticalAxisValues=taxis, child=True)
        ws = alg.getProperty("OutputWorkspace").value

        ws.getAxis(0).setUnit("Label").setLabel("Time", "ns")
        ws.getAxis(1).setUnit("Label").setLabel("Temperature", "K")

        unitx = ws.getAxis(0).getUnit()
        unity = ws.getAxis(1).getUnit()
        self.assertEquals("Time",unitx.caption())
        self.assertEquals("ns",unitx.label())
        self.assertEquals("Temperature",unity.caption())
        self.assertEquals("K",unity.label())

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
        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=3,
                            VerticalAxisUnit="Text",VerticalAxisValues=axis_values,child=True)
        workspace = alg.getProperty("OutputWorkspace").value
        txtAxis = workspace.getAxis(1)
        self.assertTrue(txtAxis.isText())
        values = txtAxis.extractValues()
        self.assertTrue(isinstance(values, list))
        for index, value in enumerate(values):
            self.assertEquals(value, axis_values[index])

if __name__ == '__main__':
    unittest.main()
