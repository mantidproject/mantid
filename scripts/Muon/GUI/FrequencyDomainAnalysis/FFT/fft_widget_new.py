# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_view import FFTView
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new import FFTPresenter
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_model import FFTModel, FFTWrapper

from PyQt4 import QtGui


class FFTWidget(QtGui.QWidget):

    def __init__(self, load, parent=None):
        super(FFTWidget, self).__init__(parent)
        view = FFTView(parent)

        fft = FFTModel()
        model = FFTWrapper(fft)

        self._presenter = FFTPresenter(view=view, alg=model, load=load)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    def runChanged(self):
        self._presenter.runChanged()

    def closeEvent(self, event):
        self._presenter.cancel()
