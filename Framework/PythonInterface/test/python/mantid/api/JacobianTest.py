from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import Jacobian

class JacobianTest(unittest.TestCase):

    def test_class_has_expected_attrs(self):
        self.assertTrue(hasattr(Jacobian, 'set'), "No set method found on Jacobian class")
        self.assertTrue(hasattr(Jacobian, 'get'), "No get method found on Jacobian class")

if __name__ == '__main__':
    unittest.main()
