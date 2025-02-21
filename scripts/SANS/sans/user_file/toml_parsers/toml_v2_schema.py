# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.user_file.toml_parsers.toml_base_schema import TomlSchemaValidator
from sans.user_file.toml_parsers.toml_v1_schema import TomlSchemaV1Validator


class TomlSchemaV2Validator(TomlSchemaValidator):
    # As of the current TOML release there is no way to validate a schema so
    # we must provide an implementation

    def __init__(self, dict_to_validate):
        super(TomlSchemaV2Validator, self).__init__(dict_to_validate)

    @staticmethod
    def reference_schema():
        """
        Returns a dictionary layout of all supported keys. Extends from the V1 Schema.
        :return: Dictionary containing all keys, and values set to None
        """
        component_keys = {
            "idf_component_name": None,
            "device_name": None,
            "device_type": None,
            "location": {"x", "y", "z"},
            "transmission": None,
            "efficiency": None,
        }
        filter_keys = dict(
            component_keys,
            **{
                "cell_length": None,
                "gas_pressure": None,
                "empty_cell": None,
                "initial_polarization": None,
            },
        )
        field_keys = {
            "sample_strength_log": None,
            "sample_direction": {"a", "p", "d"},
            "sample_direction_log": None,
        }
        polarization_keys = {
            "flipper_configuration": None,
            "spin_configuration": None,
            "flipper": {"*": component_keys},
            "polarizer": filter_keys,
            "analyzer": filter_keys,
            "magnetic_field": field_keys,
            "electric_field": field_keys,
        }

        return dict(TomlSchemaV1Validator.reference_schema(), **{"polarization": polarization_keys})
