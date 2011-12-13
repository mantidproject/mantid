import unittest
from testhelpers import can_be_instantiated
from mantid import IComponent

class IComponentTest(unittest.TestCase):
    
    def test_IComponent_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(IComponent))
