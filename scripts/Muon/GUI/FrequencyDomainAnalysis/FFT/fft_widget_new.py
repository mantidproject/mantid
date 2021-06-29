# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_view_new import FFTView
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new import FFTPresenter
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_model import FFTModel, FFTWrapper

from qtpy import QtWidgets


class FFTWidget(QtWidgets.QWidget):

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

    def update_view_from_model(self):
        self._presenter.update_view_from_model()
