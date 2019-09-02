# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import UnitFactory, UnitLabel
import six
import sys

class UnitLabelTest(unittest.TestCase):

    def test_UnitLabel_can_be_built_from_simple_string(self):
      label = UnitLabel("MyLabel")
      self.assertEqual("MyLabel", label.ascii())

    def test_UnitLabel_can_be_built_simple_string_and_unicode_object(self):
        label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
        self.assertEqual("MyLabel", label.ascii())
        self.assertEqual(u"\u03bcs", label.utf8())
        self.assertEqual("\mu s", label.latex())

    def test_utf8_is_converted_to_unicode_object(self):
        tof = UnitFactory.Instance().create("TOF")
        unit_lbl = tof.symbol()
        self.assertTrue(isinstance(unit_lbl.utf8(), six.text_type))
        self.assertEqual(u"\u03bcs", unit_lbl.utf8())
        self.assertEqual("\mu s", unit_lbl.latex())

    def test_str_function_produces_ascii_string_from_label(self):
        label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
        self.assertTrue(isinstance(str(label), six.string_types))
        self.assertEqual("MyLabel", str(label))

    def test_unicode_function_produces_unicode_string_from_label_py2(self):
        if sys.version_info[0] < 3:
            label = UnitLabel("MyLabel", u"\u03bcs","\mu s")
            self.assertTrue(isinstance(unicode(label), six.text_type))
            self.assertEqual(u"\u03bcs", unicode(label))
        else:
            pass

if __name__ == '__main__':
    unittest.main()
