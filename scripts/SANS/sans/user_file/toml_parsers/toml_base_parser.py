# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABCMeta, abstractmethod

from sans.state.IStateParser import IStateParser
from sans.user_file.toml_parsers.toml_base_schema import TomlSchemaValidator


class TomlParserBase(IStateParser, metaclass=ABCMeta):
    def __init__(self, dict_to_parse, file_information, schema_validator: TomlSchemaValidator):
        self._validator = schema_validator
        self._validator.validate()

        data_info = self.get_state_data(file_information)
        self._implementation = self._get_impl(dict_to_parse, data_info)
        self._implementation.parse_all()

    @staticmethod
    @abstractmethod
    def _get_impl(*args):
        pass
