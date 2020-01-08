# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.user_file.txt_parsers.ParsedDictConverter import ParsedDictConverter
from sans.user_file.user_file_reader import UserFileReader


class UserFileParserAdapter(ParsedDictConverter):
    def __init__(self, filename, txt_user_file_reader: UserFileReader = None):
        super(UserFileParserAdapter, self).__init__()

        if not txt_user_file_reader:
            txt_user_file_reader = UserFileReader(filename)

        self._adapted_parser = txt_user_file_reader

    def _get_input_dict(self) -> dict:
        return self._adapted_parser.read_user_file()
