# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.user_file.toml_parsers.toml_v2_schema import TomlSchemaV2Validator
from sans.user_file.toml_parsers.toml_base_schema import TomlValidationError


class SchemaV2ValidatorTest(unittest.TestCase):
    def test_multiple_flippers_respected(self):
        valid_example = {"polarization": {"flipper": {"F0": {"device_type": "coil"}, "F1": {"device_type": "magic"}}}}
        invalid_example = {"polarization": {"flipper": {"F0": {"device_type": "coil"}}}, "flipper": {"F1": {"fake_key": "magic"}}}

        obj = TomlSchemaV2Validator(valid_example)
        self.assertIsNone(obj.validate())

        with self.assertRaises(TomlValidationError):
            TomlSchemaV2Validator(invalid_example).validate()

    def test_duplicate_flippers_fails(self):
        invalid_example = {"polarization": {"flipper": {"F0": {"device_type": "coil"}}}, "flipper": {"F0": {"device_type": "magic"}}}

        with self.assertRaises(TomlValidationError):
            TomlSchemaV2Validator(invalid_example).validate()


if __name__ == "__main__":
    unittest.main()
