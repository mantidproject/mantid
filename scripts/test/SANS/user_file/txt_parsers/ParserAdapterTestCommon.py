# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import mock

from sans.user_file.txt_parsers.ParserAdapter import ParserAdapter
from sans.user_file.user_file_reader import UserFileReader


class ParserAdapterTestCommon(object):
    def set_return_val(self, val):
        self.mocked_adapted = mock.Mock(autospec=UserFileReader)
        self.mocked_adapted.read_user_file.return_value = val
        self.instance = ParserAdapter(filename="ABC", txt_user_file_reader=self.mocked_adapted)
