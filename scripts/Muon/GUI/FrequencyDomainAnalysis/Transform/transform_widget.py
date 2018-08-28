from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_view import TransformView

from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget
from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_widget import TransformSelectionWidget

from qtpy import QtWidgets


class TransformWidget(QtWidgets.QWidget):
    def __init__(self, load, parent=None):
        super(TransformWidget, self).__init__(parent)
        self._fft = FFTWidget(load=load, parent=self)
        self._maxent = MaxEntWidget(load=load, parent=self)
        self._selector = TransformSelectionWidget(parent=self)

        groupedViews = self.getViews()

        self._view = TransformView(self._selector.widget, groupedViews, parent)

        self._selector.setSelectionConnection(self.updateDisplay)

    @property
    def widget(self):
        return self._view

    def mockWidget(self, mockView):
        self._view = mockView

    def closeEvent(self, event):
        self._selector.closeEvent(event)
        self._fft.closeEvent(event)
        self._maxent.closeEvent(event)

    def updateDisplay(self, method):
        self._view.hideAll()
        self._view.show(method)

    def getViews(self):
        groupedViews = {}
        groupedViews["FFT"] = self._fft.widget
        groupedViews["MaxEnt"] = self._maxent.widget
        return groupedViews
