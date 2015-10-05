import unittest
from mantid.kernel import UnitFactory, UnitLabel
import types

class UnitLabelTest(unittest.TestCase):

    def test_UnitLabel_can_be_built_from_simple_string(self):
      label = UnitLabel("MyLabel")
      self.assertEquals("MyLabel", label.ascii())

    def test_UnitLabel_can_be_built_simple_string_and_unicode_object(self):
        label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
        self.assertEquals("MyLabel", label.ascii())
        self.assertEquals(u"\u03bcs", label.utf8())
        self.assertEquals("\mu s", label.latex())

    def test_utf8_is_converted_to_unicode_object(self):
        tof = UnitFactory.Instance().create("TOF")
        unit_lbl = tof.symbol()
        self.assertTrue(isinstance(unit_lbl.utf8(), types.UnicodeType))
        self.assertEquals(u"\u03bcs", unit_lbl.utf8())
        self.assertEquals("\mu s", unit_lbl.latex())

    def test_str_function_produces_ascii_string_from_label(self):
        label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
        self.assertTrue(isinstance(str(label), types.StringType))
        self.assertEquals("MyLabel", str(label))

    def test_unicode_function_produces_unicode_string_from_label(self):
        label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
        self.assertTrue(isinstance(unicode(label), types.UnicodeType))
        self.assertEquals(u"\u03bcs", unicode(label))

if __name__ == '__main__':
    unittest.main()
