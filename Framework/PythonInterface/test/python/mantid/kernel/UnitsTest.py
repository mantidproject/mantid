import unittest
from mantid.kernel import UnitFactory, Unit, Label, UnitLabel
import types

class UnitsTest(unittest.TestCase):

    def test_Label_is_returned_from_Factory(self):
        label_unit = UnitFactory.Instance().create("Label")
        self.assertTrue(isinstance(label_unit, Unit))
        self.assertTrue(isinstance(label_unit, Label))
        label_unit.setLabel("Temperature", "K")
        self.assertEquals("Temperature", label_unit.caption())
        self.assertEquals("K", label_unit.label())
        self.assertTrue(isinstance(label_unit.symbol(), UnitLabel))

if __name__ == '__main__':
    unittest.main()
