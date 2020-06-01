# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
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

    def get_input_dict(self):
        input_dict = {
            'binning_type': self.cbBinningType.currentIndex(),
            'single_low': self.spSingleLow.value(),
            'single_width': self.spSingleWidth.value(),
            'single_high': self.spSingleHigh.value(),
            'multiple_bin': self.leMultiple.text(),
            'chemical_formula': self.leChemicalFormula.text(),
            'density_type': self.cbDensity.currentText(),
            'density': self.spDensity.value(),
            'thickness': self.spThickness.value()
        }
        return input_dict

    def set_output_table(self, output_dict, statistics):
        self.twResults.clear()
        scattering_item = QtWidgets.QTreeWidgetItem()
        scattering_item.setText(0, 'Scattering')
        scattering_item.setText(1, str(statistics))
        self.twResults.addTopLevelItem(scattering_item)

        transmission_item = QtWidgets.QTreeWidgetItem()
        transmission_item.setText(0, "Transmission")
        self.twResults.addTopLevelItem(transmission_item)
        transmission_item.setExpanded(True)

        for key in output_dict:
            item = QtWidgets.QTreeWidgetItem()
            item.setText(0, key)
            item.setText(1, str(output_dict[key]))
            transmission_item.addChild(item)

    def plot(self, x, y):
        print(x)
        print(y)
        self.axes.cla()
        self.axes.plot(x, y)
        self.plot_frame_2.draw()
