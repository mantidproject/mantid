from __future__ import (absolute_import, division, print_function)


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
