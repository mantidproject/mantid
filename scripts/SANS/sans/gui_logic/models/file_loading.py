# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans.user_file.toml_parsers.toml_parser import TomlParser
from sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class FileLoading:
    @staticmethod
    def load_user_file(file_path, file_information):
        """
        Loads a user file into State Objects for both legacy .txt files and .TOML files
        file_path: The full path to the file to load
        instrument: The instrument name
        """
        if file_path.casefold().endswith("TOML".casefold()):
            parser = TomlParser()
            return parser.parse_toml_file(file_path, file_information=file_information)
        else:
            converter = UserFileReaderAdapter(user_file_name=file_path, file_information=file_information)
            return converter.get_all_states(file_information=file_information)
