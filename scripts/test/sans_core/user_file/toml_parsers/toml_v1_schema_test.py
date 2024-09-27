# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans_core.user_file.toml_parsers.toml_v1_schema import TomlSchemaV1Validator, TomlValidationError


class SchemaV1ValidatorTest(unittest.TestCase):
    def test_paths_build_for_nested_dict(self):
        test_grid = [({"A": None}, ["A"]), ({"A": {"B": None}}, ["A.B"]), ({"A": {"B": None, "C": None}}, ["A.B", "A.C"])]

        for test_in, expected in test_grid:
            output = TomlSchemaV1Validator._build_nested_keys(test_in)
            self.assertEqual(expected, output)

    def test_path_building_can_handle_set(self):
        expected = ["A.Set", "A.SetB"]
        output = TomlSchemaV1Validator._build_nested_keys({"A": {"Set", "SetB"}})

        # These can come in any order - so to avoid flaky tests we assert they are in
        self.assertTrue(expected[0] in output)
        self.assertTrue(expected[1] in output)

    def test_set_and_dict_paths_are_equiv(self):
        input_a = {"A": {"B": None}}
        input_b = {"A": {"B"}}

        output_a = TomlSchemaV1Validator._build_nested_keys(input_a)
        output_b = TomlSchemaV1Validator._build_nested_keys(input_b)
        self.assertEqual(output_a, output_b)

    def test_throws_if_unrecognised_top_level_key(self):
        for i in [{"NotRecognised": None}, {"Foo": {"Bar": None}}]:
            obj = TomlSchemaV1Validator(i)
            with self.assertRaises(TomlValidationError):
                obj.validate()

    def test_all_unknown_keys_mentioned(self):
        obj = TomlSchemaV1Validator({"A": None, "B": None})
        with self.assertRaises(TomlValidationError) as e:
            obj.validate()
            self.assertTrue("A" in e)
            self.assertTrue("B" in e)

    def test_sub_key_checked(self):
        obj = TomlSchemaV1Validator({"instrument": "Foo"})
        with self.assertRaises(TomlValidationError):
            obj.validate()

    def test_valid_key_accepted(self):
        expected_valid_examples = [
            {"instrument": {"name": mock.NonCallableMock()}},
            {"instrument": {"configuration": {"sample_offset": 1.0}}},
        ]

        for i in expected_valid_examples:
            obj = TomlSchemaV1Validator(i)
            self.assertIsNone(obj.validate())

    def test_wildcard_respected(self):
        valid_example = {"transmission": {"monitor": {"FOO": {"spectrum_number": None}}}}
        invalid_example = {"transmission": {"monitor": {"FOO": {"NotRecognisedKey": None}}}}

        obj = TomlSchemaV1Validator(valid_example)
        self.assertIsNone(obj.validate())

        with self.assertRaises(TomlValidationError):
            TomlSchemaV1Validator(invalid_example).validate()


if __name__ == "__main__":
    unittest.main()
