from __future__ import (absolute_import, division, print_function)

from Muon.transform_view import TransformView
from Muon.transform_presenter import TransformPresenter

from Muon.fft_widget import FFTWidget
from Muon.maxent_widget import MaxEntWidget
from Muon.transform_selection_widget import TransformSelectionWidget

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

    def updateDisplay(self,method):
        self.view.hideAll()
        self.view.show(method)

    def getViews(self):
        groupedViews = {}
        groupedViews["FFT"] = self.fft.widget
        groupedViews["MaxEnt"] = self.maxent.widget
        return groupedViews
