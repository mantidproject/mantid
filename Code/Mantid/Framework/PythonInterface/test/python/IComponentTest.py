import unittest
from testhelpers import can_be_instantiated
from mantid import IComponent

class IComponentTest(unittest.TestCase):
    
    def test_IComponent_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(IComponent))
        
    def test_IComponent_has_expected_attributes(self):
        attrs = dir(IComponent)
        expected_attrs = ["get_pos", "get_distance", "get_name", "type"]
        for att in expected_attrs:
            self.assertTrue(att in attrs)
