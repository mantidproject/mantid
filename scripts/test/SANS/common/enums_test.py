# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from sans.common.enums import SANSEnum


# ----Create a test class
DummyClass = SANSEnum("DummyClass", "TypeA TypeB TypeC")
IncorrectClass = SANSEnum("IncorrectClass", "TypeA TypeB TypeC")


class SANSEnumsTest(unittest.TestCase):
    def test_that_has_member_handles_strings(self):
        self.assertTrue(DummyClass.has_member("TypeA"))
        self.assertFalse(DummyClass.has_member("TypeD"))

    def test_that_has_member_handles_enums(self):
        a_variable = DummyClass.TypeA
        incorrect_variable = IncorrectClass.TypeA

        self.assertTrue(DummyClass.has_member(a_variable))
        self.assertFalse(DummyClass.has_member(incorrect_variable))

    def test_that_enum_is_initialised_correctly(self):
        try:
            dummy = DummyClass.TypeA
            dummy = DummyClass.TypeB
            dummy = DummyClass.TypeC
        except AttributeError as e:
            self.fail(str(e))


if __name__ == '__main__':
    unittest.main()
