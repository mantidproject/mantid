from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import IPropertySettings

class IPropertySettingsTest(unittest.TestCase):

    def test_construction_raises_an_exception(self):
        self.assertRaises(RuntimeError, IPropertySettings)

    def test_interface_has_expected_attrs(self):
        self.assertTrue(hasattr(IPropertySettings, "isEnabled"))
        self.assertTrue(hasattr(IPropertySettings, "isVisible"))

if __name__ == '__main__':
    unittest.main()
