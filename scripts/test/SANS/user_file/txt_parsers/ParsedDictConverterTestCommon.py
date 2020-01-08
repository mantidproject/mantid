# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.user_file.txt_parsers.ParsedDictConverter import ParsedDictConverter


class ParsedDictConverterTestCommon(object):

    def set_return_val(self, val):

        class AdapterShim(ParsedDictConverter):
            def _get_input_dict(self) -> dict:
                return val

        self.instance = AdapterShim()
