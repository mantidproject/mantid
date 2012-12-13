import unittest
from mantid.geometry import Instrument
from testhelpers import can_be_instantiated

class InstrumentTest(object):
    
    def test_Instrument_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(Instrument))

    def test_Instrument_has_expected_attrs(self):
        expected_attrs = ["getSample", "getSource", 
                          "getComponentByName", "getDetector", 
                          "nelements", "__getitem__",
                          "get_parameter_names", "has_parameter",
                          "get_number_parameter", "get_position_parameter",
                          "get_rotation_parameter", "get_string_parameter",
                          "get_pos", "get_distance", "get_name", "type"]
        for att in expected_attrs:
            self.assertTrue(att in attrs)
