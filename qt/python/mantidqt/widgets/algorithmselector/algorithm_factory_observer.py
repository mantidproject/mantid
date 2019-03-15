# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import absolute_import


from mantid.api import AlgorithmFactoryObserver


class AlgorithmSelectorFactoryObserver(AlgorithmFactoryObserver):

    def __init__(self, widget):
        super(AlgorithmSelectorFactoryObserver, self).__init__()
        self.widget = widget

        self.observeUpdate(True)

    def updateHandle(self):
        self.widget.refresh()
