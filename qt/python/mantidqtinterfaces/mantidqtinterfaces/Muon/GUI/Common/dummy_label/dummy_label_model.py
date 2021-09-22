# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class DummyLabelModel(object):

    def __init__(self, context, key):
        self._context = context
        self._key = key

    def getSubContext(self):
        subContext = {}
        subContext["label"] = self._context.get(self._key)
        return subContext

    def updateContext(self, subContext):
        self._context.set(self._key, subContext["label"])
