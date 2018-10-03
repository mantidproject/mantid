# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtpython import MantidQt


class BasicHintStrategy(MantidQt.MantidWidgets.HintStrategy):
    def __init__(self, hint_dict):
        super(BasicHintStrategy, self).__init__()
        self.hint_dict = hint_dict

    def createHints(self):
        hint_list = []
        for key, item in self.hint_dict.items():
            new_hint = MantidQt.MantidWidgets.Hint(key, item)
            hint_list.append(new_hint)
        return hint_list

    def __eq__(self, other):
        return self.__dict__ == other.__dict__
