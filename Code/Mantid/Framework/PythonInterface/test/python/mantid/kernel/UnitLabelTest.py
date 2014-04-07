import unittest
from mantid.kernel import UnitFactory, UnitLabel
import types

class UnitLabelTest(unittest.TestCase):

    def test_UnitLabel_can_be_built_from_simple_string(self):
      label = UnitLabel("MyLabel")
      self.assertEquals("MyLabel", label.ascii())

    def test_UnitLabel_can_be_built_simple_string_and_unicode_object(self):
      label = UnitLabel("MyLabel", u"\u03bcs")
      self.assertEquals("MyLabel", label.ascii())
      self.assertEquals(u"\u03bcs", label.utf8())

    def test_utf8_is_converted_to_unicode_object(self):
        tof = UnitFactory.Instance().create("TOF")
        unit_lbl = tof.label()
        self.assertTrue(isinstance(unit_lbl.utf8(), types.UnicodeType))
        self.assertEquals(u"\u03bcs", unit_lbl.utf8())

if __name__ == '__main__':
    unittest.main()
