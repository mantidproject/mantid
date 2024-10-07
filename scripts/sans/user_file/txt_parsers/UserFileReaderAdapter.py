# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans.user_file.txt_parsers.ParsedDictConverter import ParsedDictConverter
from sans.user_file.user_file_reader import UserFileReader


class UserFileReaderAdapter(ParsedDictConverter):
    def __init__(self, file_information, user_file_name, txt_user_file_reader=None):
        if not txt_user_file_reader:
            txt_user_file_reader = UserFileReader(user_file_name)

        self._adapted_parser = txt_user_file_reader

        # Now we can let the superclass call us
        super(UserFileReaderAdapter, self).__init__(file_information=file_information)

    def _get_input_dict(self):  # -> dict:
        return self._adapted_parser.read_user_file()
