from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.common.enums import serializable_enum, string_convertible


# ----Create a test class
@string_convertible
@serializable_enum("TypeA", "TypeB", "TypeC")
class DummyClass(object):
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

if __name__ == '__main__':
    unittest.main()
