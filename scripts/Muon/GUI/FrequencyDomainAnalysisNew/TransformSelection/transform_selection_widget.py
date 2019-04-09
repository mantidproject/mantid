# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysisNew.TransformSelection.transform_selection_view import TransformSelectionView
from Muon.GUI.FrequencyDomainAnalysisNew.TransformSelection.transform_selection_presenter import TransformSelectionPresenter

from PyQt4 import QtGui


class TransformSelectionWidget(QtGui.QWidget):

    def __init__(self, parent=None):
        super(TransformSelectionWidget, self).__init__(parent)
        view = TransformSelectionView(parent)
        self._presenter = TransformSelectionPresenter(view)

    def setSelectionConnection(self, slot):
        view = self.widget
        view.changeMethodSignal.connect(slot)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget
