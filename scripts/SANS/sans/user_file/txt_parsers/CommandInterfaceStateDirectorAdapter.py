# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.user_file.txt_parsers.ParsedDictConverter import ParsedDictConverter


class CommandInterfaceStateDirectorAdapter(ParsedDictConverter):
    def __init__(self, parsed_dict):
        super(CommandInterfaceStateDirectorAdapter, self).__init__()

        assert(isinstance(parsed_dict, dict))
        self._parsed_dict = parsed_dict

    def _get_input_dict(self) -> dict:
        return self._parsed_dict
