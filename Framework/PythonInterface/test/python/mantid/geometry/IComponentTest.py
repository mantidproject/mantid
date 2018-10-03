# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import can_be_instantiated
from mantid.geometry import IComponent

class IComponentTest(unittest.TestCase):

    def test_IComponent_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(IComponent))

    def test_IComponent_has_expected_attributes(self):
        attrs = dir(IComponent)
        expected_attrs = ["getPos", "getDistance", "getName", "type"]
        for att in expected_attrs:
            self.assertTrue(att in attrs)

if __name__ == '__main__': unittest.main()
