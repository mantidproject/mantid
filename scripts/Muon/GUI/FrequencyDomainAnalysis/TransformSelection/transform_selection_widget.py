from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_view import TransformSelectionView
from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_presenter import TransformSelectionPresenter

from PyQt4 import QtGui


class TransformSelectionWidget(QtGui.QWidget):

    def __init__(self, parent=None):
        super(TransformSelectionWidget, self).__init__(parent)
        view = TransformSelectionView(parent)
        self.presenter = TransformSelectionPresenter(view)

    def setSelectionConnection(self, slot):
        view = self.widget
        view.changeMethodSignal.connect(slot)

    @property
    def presenter(self):
        return self.presenter

    @property
    def widget(self):
        return self.presenter.widget
