# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from MultiPlotting.multiPlotting_context import *

# use axis changer object later

class QuickEditModel(object ):
    def __init__(self,context):
        self._context = context

    def getSubContext(self):
        subContext = {}
        subContext["x bounds"] = self._context.get(xBounds)
        subContext["y bounds"] = self._context.get(yBounds)
        return subContext

    def updateContext(self, subContext):
        self._context.set(xBounds, subContext["x bounds"]) 
        self._context.set(yBounds, subContext["y bounds"]) 
