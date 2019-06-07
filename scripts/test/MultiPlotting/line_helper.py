# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

class line(object):

    def __init__(self):
        self.label = "test"

    def get_label(self):
        return self.label

    def get_color(self):
        return "red"

    def get_marker(self):
        return "star"

    def remove(self):
        return


class label(object):

    def __init__(self, name, protected):
        self.text = name
        self.protected = protected


def errors():
    return tuple([line(), [line()], [line()]])


