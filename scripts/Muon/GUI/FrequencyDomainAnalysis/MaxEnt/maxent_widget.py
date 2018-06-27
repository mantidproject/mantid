from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_view import MaxEntView
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter import MaxEntPresenter
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_model import MaxEntModel, MaxEntWrapper

from PyQt4 import QtGui


class MaxEntWidget(QtGui.QWidget):

    def __init__(self, load, parent=None):
        super(MaxEntWidget, self).__init__(parent)
        view = MaxEntView(parent)

        maxEnt = MaxEntModel()
        model = MaxEntWrapper(maxEnt)
        self.pres = MaxEntPresenter(view, model, load)

    @property
    def presenter(self):
        return self.pres

    @property
    def widget(self):
        return self.pres.widget

    def closeEvent(self, event):
        self.pres.cancel()
