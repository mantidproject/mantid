from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_view import MaxEntView
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter import MaxEntPresenter
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_model import MaxEntModel, MaxEntWrapper

from qtpy import QtWidgets


class MaxEntWidget(QtWidgets.QWidget):

    def __init__(self, load, parent=None):
        super(MaxEntWidget, self).__init__(parent)
        view = MaxEntView(parent)

        maxEnt = MaxEntModel()
        model = MaxEntWrapper(maxEnt)
        self._presenter = MaxEntPresenter(view, model, load)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def closeEvent(self, event):
        self._presenter.cancel()
