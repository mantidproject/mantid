# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans_core.user_file.toml_parsers.toml_parser import TomlParser
from sans_core.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class UserFileLoadException(Exception):
    pass


class FileLoading:
    @staticmethod
    def load_user_file(file_path, file_information):
        """
        Loads a user file into State Objects for both legacy .txt files and .TOML files
        file_path: The full path to the file to load
        instrument: The instrument name
        """
        if file_path.casefold().endswith("TOML".casefold()):
            return FileLoading._parse_toml(file_path, file_information)
        else:
            return FileLoading._parse_legacy(file_path, file_information)

    @staticmethod
    def _parse_toml(file_path, file_information):
        parser = TomlParser()
        try:
            return parser.parse_toml_file(file_path, file_information=file_information)
        except KeyError as e:
            raise UserFileLoadException(f"The following key is missing: {e}")
        except (NotImplementedError, ValueError) as e:
            raise UserFileLoadException(e)

    @staticmethod
    def _parse_legacy(file_path, file_information):
        try:
            converter = UserFileReaderAdapter(user_file_name=file_path, file_information=file_information)
            return converter.get_all_states(file_information=file_information)
        except (RuntimeError, ValueError) as e:
            raise UserFileLoadException(e)
