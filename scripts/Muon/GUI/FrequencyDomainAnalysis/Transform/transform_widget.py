# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_view import TransformView

from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget
from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_widget import TransformSelectionWidget
from Muon.GUI.Common.observer_pattern import Observer, Observable

from PyQt4 import QtGui


class TransformWidget(QtGui.QWidget):
    def __init__(self, load, parent=None):
        super(TransformWidget,self).__init__(parent)
        self._fft = FFTWidget(load=load,parent=self)
        self._maxent = MaxEntWidget(load=load,parent=self)
        self._selector = TransformSelectionWidget(parent=self)
        self.LoadObserver = LoadObserver(self)
        self.instrumentObserver = instrumentObserver(self)

        groupedViews = self.getViews()

        self._view = TransformView(self._selector.widget, groupedViews,parent)

        self._selector.setSelectionConnection(self.updateDisplay)

    @property
    def widget(self):
        return self._view

    def mockWidget(self, mockView):
        self._view = mockView

    def closeEvent(self,event):
        self._selector.closeEvent(event)
        self._fft.closeEvent(event)
        self._maxent.closeEvent(event)

    def updateDisplay(self,method):
        self._view.hideAll()
        self._view.showMethod(method)

    def getViews(self):
        groupedViews = {}
        groupedViews["FFT"] = self._fft.widget
        groupedViews["MaxEnt"] = self._maxent.widget
        return groupedViews

    def handle_new_data_loaded(self):
        self._maxent.runChanged()

    def handle_new_instrument(self):
        self._maxent.clear()

class LoadObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.handle_new_data_loaded()

class instrumentObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.handle_new_instrument()