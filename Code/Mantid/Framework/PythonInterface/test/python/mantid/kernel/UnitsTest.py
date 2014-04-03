import unittest
from mantid.kernel import UnitFactory, Unit, Label
import types

class UnitsTest(unittest.TestCase):

    def test_Label_is_returned_from_Factory(self):
        label_unit = UnitFactory.Instance().create("Label")
        self.assertTrue(isinstance(label_unit, Unit))
        self.assertTrue(isinstance(label_unit, Label))
        label_unit.setLabel("Temperature", "K")
        self.assertEquals("Temperature", label_unit.caption())
        self.assertEquals("K", label_unit.label())

    def test_utf8Label_is_converted_to_unicode_object(self):
        tof = UnitFactory.Instance().create("TOF")
        unit_lbl = tof.utf8Label()
        self.assertTrue(isinstance(unit_lbl, types.UnicodeType))
        self.assertEquals(u"\u03bcs", unit_lbl)


if __name__ == '__main__':
    unittest.main()
