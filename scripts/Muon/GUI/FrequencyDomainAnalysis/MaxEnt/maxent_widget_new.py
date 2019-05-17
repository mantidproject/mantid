# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_view_new import MaxEntView
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter_new import MaxEntPresenter

from qtpy import QtWidgets


class MaxEntWidget(QtWidgets.QWidget):

    def __init__(self, load, parent=None):
        super(MaxEntWidget, self).__init__(parent)
        view = MaxEntView(parent)
        self._presenter = MaxEntPresenter(view, load)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def clear(self):
        self._presenter.clear()

    def runChanged(self):
        self._presenter.runChanged()

    def closeEvent(self, event):
        self._presenter.cancel()
