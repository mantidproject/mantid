# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.user_file.toml_parsers.toml_reader import TomlReader
from sans.user_file.toml_parsers.toml_v1_parser import TomlV1Parser


class TomlParser(object):
    def __init__(self, toml_reader = None):
        self._lib_impl = toml_reader if toml_reader else TomlReader()

    def get_toml_parser(self, toml_file_path):
        parsed_dict = self._lib_impl.get_user_file_dict(toml_file_path)
        return self.get_versioned_parser(parsed_dict)

    @staticmethod
    def get_versioned_parser(toml_dict):
        """
        Allows us to substitute in different parsers with language specs if we need to update the syntax
        by returning a parser object specific to the current version.
        :param toml_dict: A toml dictionary with the "format_version" key
        :return: Appropriate parser for the associated TOML dictionary
        """
        version = toml_dict["format_version"]
        if version == 0:
            return TomlV1Parser(toml_dict)
        else:
            raise NotImplementedError("Version {0} of the SANS Toml Format is not supported".format(version))
