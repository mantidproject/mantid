# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import isis_instrument as instruments
from mantid.api import mtd
from mantid.simpleapi import CreateSampleWorkspace, DeleteWorkspace, SetInstrumentParameter


class SANSIsisInstrumentTest(unittest.TestCase):
    def test_add_TOFs_for_ROI_is_correct(self):
        # Arrange
        start = 125
        end = 10000
        inst = instruments.LOQ()
        inst.set_TOFs_for_ROI(start, end)

        # Act
        start_Tof, end_Tof = inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(start, start_Tof)
        self.assertEqual(end, end_Tof)

    def test_Tofs_for_ROI_are_reset_to_None(self):
        # Arrange
        start = 125
        end = 10000
        inst = instruments.LOQ()
        inst.set_TOFs_for_ROI(start, end)
        inst.reset_TOFs_for_ROI()

        # Act
        start_Tof, end_Tof = inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(None, start_Tof)
        self.assertEqual(None, end_Tof)


class TestParameterMapModifications(unittest.TestCase):
    def create_sample_workspace(self, ws_name, types, values, names):
        ws = CreateSampleWorkspace(OutputWorkspace=ws_name)
        length = len(types)
        if any(len(element) != length for element in [types, values, names]):
            return
        for i in range(0, length):
            SetInstrumentParameter(ws, ParameterName=names[i], ParameterType=types[i], Value=values[i])

    def test_that_calibration_workspace_has_absent_entries_copied_over(self):
        # Arrange
        ws_name1 = "ws1"
        types1 = ["Number", "String", "Number"]
        values1 = ["100.4", "test", "200"]
        names1 = ["val1", "val2", "val3"]
        self.create_sample_workspace(ws_name1, types1, values1, names1)

        ws_name2 = "ws2"
        types2 = ["Number", "String", "Number"]
        values2 = ["2.4", "test3", "3"]
        names2 = ["val1", "val2", "val4"]
        self.create_sample_workspace(ws_name2, types2, values2, names2)

        inst = instruments.SANS2D()

        # Act
        inst._add_parmeters_absent_in_calibration(ws_name1, ws_name2)

        # Assert
        ws2 = mtd[ws_name2]
        self.assertTrue(ws2.getInstrument().hasParameter("val1"))
        self.assertTrue(ws2.getInstrument().hasParameter("val2"))
        self.assertTrue(ws2.getInstrument().hasParameter("val3"))
        self.assertTrue(ws2.getInstrument().hasParameter("val4"))

        self.assertEqual(len(ws2.getInstrument().getNumberParameter("val1")), 1)
        self.assertEqual(ws2.getInstrument().getNumberParameter("val1")[0], 2.4)

        self.assertEqual(len(ws2.getInstrument().getIntParameter("val3")), 1)
        self.assertEqual(ws2.getInstrument().getIntParameter("val3")[0], 200)

        # Clean up
        DeleteWorkspace(ws_name1)
        DeleteWorkspace(ws_name2)


if __name__ == "__main__":
    unittest.main()
