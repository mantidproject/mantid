# pylint: disable=invalid-name, too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import DateAndTime
from mantid.api import EventType
from mantid.api import FunctionFactory
# from mantid._plugins import ProductFunction

class ProductFunctionTest(unittest.TestCase):

    def test_length(self):
        p = FunctionFactory.createFunction("ProductFunction")
        self.assertEquals(len(p), 0)
        
    def test_addition(self):
        p = FunctionFactory.createFunction("ProductFunction")
        g = FunctionFactory.createFunction("Gaussian")
        p.add(g)
        p.add(g)
        self.assertEquals(len(p), 2)

if __name__ == '__main__':
    unittest.main()
