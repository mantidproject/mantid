# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets
from matplotlib.figure import Figure
from mantidqt.utils.qt import load_ui
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
Ui_sample_transmission, _ = load_ui(__file__, "SampleTransmission.ui")


class SampleTransmissionCalculatorView(QtWidgets.QWidget, Ui_sample_transmission):
    def __init__(self, parent=None):
        super(SampleTransmissionCalculatorView, self).__init__(parent)
        self.setupUi(self)
        fig = Figure()
        self.axes = fig.add_subplot(111)
        self.plot_frame_2 = FigureCanvas(fig)
        self.outputLayout.addWidget(self.plot_frame_2)
        self.draw()
        #self.plot_frame = FigureCanvas(self.figure)

    def draw(self):
        ax = self.axes
        ax.clear()
        ax.set_xlabel("time ($s$)")
        ax.set_ylabel("$f(t)$")
        return ax