# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import pythonTSV as TSVHelper


class group(object):

    def __init__(self, name="", dets=[]):
        self._name = name
        self._dets = dets

    @property
    def name(self):
        return self._name

    @property
    def dets(self):
        return self._dets

    def Print(self):
        print(self._name, self._dets)

    def setName(self, name):
        self._name = name

    def setDets(self, dets):
        self._dets = dets

    def isValid(self):
        if self._name != "" and len(self._dets):
            return True

        return False

    def save(self, TSV):
        TSVHelper.writeLine(TSV, self._name)
        TSV.storeInt(len(self._dets))
        for detector in self._dets:
            TSV.storeInt(detector)

    def load(self, TSV, name):
        self._name = name
        safeName = TSVHelper.makeLineNameSafe(name)
        TSV.selectLine(safeName)
        num = TSV.readInt()
        self._dets = []
        for k in range(num):
            self._dets.append(TSV.readInt())
