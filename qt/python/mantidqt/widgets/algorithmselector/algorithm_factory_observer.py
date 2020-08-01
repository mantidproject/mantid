# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.api import AlgorithmFactoryObserver


class AlgorithmSelectorFactoryObserver(AlgorithmFactoryObserver):
    """
    Observe updates to the AlgorithmFactory and refresh the held widget
    """
    def __init__(self, widget):
        """
        :param widget: Widget with a refresh attribute
        """
        super().__init__()
        self._widget = widget
        self.observeUpdate(True)

    def updateHandle(self):
        """Called when the algorithm factory has been updated"""
        self._widget.refresh()
