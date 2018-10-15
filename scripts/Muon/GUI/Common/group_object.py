# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

class group(object):

    def __init__(self, name="", dets=[]):
        self._name = name
        self._dets = dets

    @property
    def name(self):
        return self._name

    def setName(self, name):
        self._name = name

    def setDets(self, dets):
        self._dets = dets

    def isValid(self):
        if self._name != "" and len(self._dets):
            return True

        return False
