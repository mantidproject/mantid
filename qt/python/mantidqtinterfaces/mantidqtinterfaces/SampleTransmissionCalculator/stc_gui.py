# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqtinterfaces.SampleTransmissionCalculator.stc_model import SampleTransmissionCalculatorModel
from mantidqtinterfaces.SampleTransmissionCalculator.stc_view import SampleTransmissionCalculatorView
from mantidqtinterfaces.SampleTransmissionCalculator.stc_presenter import SampleTransmissionCalculatorPresenter


class SampleTransmissionCalculator(QtWidgets.QMainWindow):
    def __init__(self, parent=None, window_flags=None):
        super(SampleTransmissionCalculator, self).__init__(parent)
        if window_flags:
            self.setWindowFlags(window_flags)

        view = SampleTransmissionCalculatorView(parent=self)
        self.setCentralWidget(view)
        model = SampleTransmissionCalculatorModel()
        self.presenter = SampleTransmissionCalculatorPresenter(view, model)
        self.setWindowTitle("Sample Transmission Calculator")
