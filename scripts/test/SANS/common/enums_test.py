# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest

from sans.common.enums import serializable_enum, string_convertible


# ----Create a test class
@string_convertible
@serializable_enum("TypeA", "TypeB", "TypeC")
class DummyClass(object):
    pass


@string_convertible
@serializable_enum("TypeA", "TypeB", "TypeC")
class IncorrectClass(object):
    pass


class SANSFileInformationTest(unittest.TestCase):
    def test_that_can_create_enum_value_and_is_sub_class_of_base_type(self):
        type_a = DummyClass.TypeA
        self.assertTrue(issubclass(type_a, DummyClass))

    def test_that_can_convert_to_string(self):
        type_b = DummyClass.TypeB
        self.assertTrue(DummyClass.to_string(type_b) == "TypeB")

    def test_that_raises_run_time_error_if_enum_value_is_not_known(self):
        self.assertRaises(RuntimeError, DummyClass.to_string, DummyClass)

    def test_that_can_convert_from_string(self):
        self.assertTrue(DummyClass.from_string("TypeC") is DummyClass.TypeC)

    def test_that_raises_run_time_error_if_string_is_not_known(self):
        self.assertRaises(RuntimeError, DummyClass.from_string, "TypeD")

    def test_that_has_member_handles_strings(self):
        self.assertTrue(DummyClass.has_member("TypeA"))
        self.assertFalse(DummyClass.has_member("TypeD"))

    def test_that_has_member_handles_enums(self):
        a_variable = DummyClass.TypeA
        incorrect_variable = IncorrectClass.TypeA

        self.assertTrue(DummyClass.has_member(a_variable))
        self.assertFalse(DummyClass.has_member(incorrect_variable))


if __name__ == '__main__':
    unittest.main()
