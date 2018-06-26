from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_view import TransformView

from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget
from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_widget import TransformSelectionWidget

from PyQt4 import QtGui


class TransformWidget(QtGui.QWidget):
    def __init__(self, load, parent=None):
        super(TransformWidget,self).__init__(parent)
        self.fft = FFTWidget(load=load,parent=self)
        self.maxent = MaxEntWidget(load=load,parent=self)
        self.selector = TransformSelectionWidget(parent=self)

        groupedViews = self.getViews()

        self.view = TransformView(self.selector.widget, groupedViews,parent)

        self.selector.setSelectionConnection(self.updateDisplay)

    @property
    def widget(self):
        return self.view

    def mockWidget(self, mockView):
        self.view = mockView

    def closeEvent(self,event):
        self.selector.closeEvent(event)
        self.fft.closeEvent(event)
        self.maxent.closeEvent(event)

    def updateDisplay(self,method):
        self.view.hideAll()
        self.view.show(method)

    def getViews(self):
        groupedViews = {}
        groupedViews["FFT"] = self.fft.widget
        groupedViews["MaxEnt"] = self.maxent.widget
        return groupedViews
