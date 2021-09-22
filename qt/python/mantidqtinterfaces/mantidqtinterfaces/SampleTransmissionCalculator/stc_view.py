# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets
from matplotlib.figure import Figure
from mantidqt.utils.qt import load_ui
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
Ui_sample_transmission, _ = load_ui(__file__, "SampleTransmission.ui")


class SampleTransmissionCalculatorView(QtWidgets.QWidget, Ui_sample_transmission):
    def __init__(self, parent=None):
        super(SampleTransmissionCalculatorView, self).__init__(parent)
        self.setupUi(self)
        fig = Figure()
        self.axes = fig.add_subplot(111)
        self.plot_frame = FigureCanvas(fig)
        self.output_layout.replaceWidget(self.placeholder_widget, self.plot_frame)
        self.assistant_process = QtCore.QProcess(self)
        self.validation_label.setStyleSheet("QLabel { color : red; }")
        self.histogram_err.setStyleSheet("QLabel { color : red; }")
        self.chemical_formula_err.setStyleSheet("QLabel { color : red; }")
        self.density_err.setStyleSheet("QLabel { color : red; }")
        self.thickness_err.setStyleSheet("QLabel { color : red; }")

    def get_input_dict(self):
        input_dict = {
            'binning_type': self.binning_type_combo_box.currentIndex(),
            'single_low': self.single_low_spin_box.value(),
            'single_width': self.single_width_spin_box.value(),
            'single_high': self.single_high_spin_box.value(),
            'multiple_bin': self.multiple_line_edit.text(),
            'chemical_formula': self.chemical_formula_line_edit.text(),
            'density_type': self.density_combo_box.currentText(),
            'density': self.density_spin_box.value(),
            'thickness': self.thickness_spin_box.value()
        }
        return input_dict

    def set_output_table(self, output_dict, statistics):
        self.results_tree.clear()
        scattering_item = QtWidgets.QTreeWidgetItem()
        scattering_item.setText(0, 'Scattering')
        scattering_item.setText(1, str(statistics))
        self.results_tree.addTopLevelItem(scattering_item)

        transmission_item = QtWidgets.QTreeWidgetItem()
        transmission_item.setText(0, "Transmission")
        self.results_tree.addTopLevelItem(transmission_item)
        transmission_item.setExpanded(True)

        for key in output_dict:
            item = QtWidgets.QTreeWidgetItem()
            item.setText(0, key)
            item.setText(1, str(output_dict[key]))
            transmission_item.addChild(item)

    def plot(self, x, y):
        self.axes.cla()
        self.axes.plot(x, y)
        self.plot_frame.figure.tight_layout()
        self.plot_frame.draw()

    def set_validation_label(self, warning_text=''):
        self.validation_label.setText(warning_text)

    def clear_error_indicator(self):
        self.histogram_err.setText('')
        self.chemical_formula_err.setText('')
        self.density_err.setText('')
        self.thickness_err.setText('')

    def set_error_indicator(self, error_key):
        getattr(self, error_key+'_err').setText('*')
