# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest

# from mantid.kernel import DateAndTime
# from mantid.api import EventType
from mantid.api import CompositeFunction, FunctionFactory
from mantid._plugins import _curvefitting

class ProductFunctionTest(unittest.TestCase):

    def test_type(self):
        p = FunctionFactory.createFunction("ProductFunction")
        self.assertTrue( isinstance(p,_curvefitting.ProductFunction) )
        self.assertTrue( isinstance(p,CompositeFunction) )

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
