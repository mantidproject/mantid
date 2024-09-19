# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from SANS.sans.state.AllStates import AllStates
from SANS.sans.state.IStateParser import IStateParser
from sans.user_file.toml_parsers.toml_reader import TomlReader
from sans.user_file.toml_parsers.toml_v1_parser import TomlV1Parser


class TomlParser(object):
    def __init__(self, toml_reader=None):
        self._lib_impl = toml_reader if toml_reader else TomlReader()

    def get_toml_parser(self, toml_file_path, file_information) -> IStateParser:
        parsed_dict = self._lib_impl.get_user_file_dict(toml_file_path)
        return self.get_versioned_parser(parsed_dict, file_information)

    def parse_toml_file(self, toml_file_path, file_information) -> AllStates:
        parser = self.get_toml_parser(toml_file_path=toml_file_path, file_information=file_information)
        return parser.get_all_states(file_information=file_information)

    @staticmethod
    def get_versioned_parser(toml_dict, file_information) -> IStateParser:
        """
        Allows us to substitute in different parsers with language specs if we need to update the syntax
        by returning a parser object specific to the current version.
        :param toml_dict: A toml dictionary with the "format_version" key
        :return: Appropriate parser for the associated TOML dictionary
        """
        version = toml_dict["toml_file_version"]
        if version == 1:
            return TomlV1Parser(toml_dict, file_information=file_information)
        else:
            raise NotImplementedError("Version {0} of the SANS Toml Format is not supported".format(version))
