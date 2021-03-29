# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.Common.message_box import warning
from Muon.GUI.Common.utilities.muon_file_utils import show_file_browser_and_return_selection

class EAAutoTabView(QtWidgets.QWidget):

    def __init__(self, match_table, parent = None):
        super(EAAutoTabView, self).__init__(parent)

        self.match_table = match_table
        self.find_peaks_notifier = GenericObservable()
        self.show_peaks_table_notifier = GenericObservable()
        self.show_match_table_notifier = GenericObservable()

        self.setup_interface_layout()
        self.setup_horizontal_layouts()
        self.setup_buttons()
        self.setup_intial_parameters()

        option = ["Workspace1" , "Workspace2" , "Workspace3"]
        self.add_options_to_find_peak_combobox(option)
        self.add_options_to_show_matches_combobox(option)
        self.add_options_to_show_peak_combobox(option)


    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.select_workspace_hlayout = QtWidgets.QHBoxLayout()
        self.select_workspace_hlayout.setObjectName("horizontalLayout1")

        self.find_peaks_parameter_hlayout = QtWidgets.QHBoxLayout()
        self.find_peaks_parameter_hlayout.setObjectName("horizontalLayout2")

        self.show_peaks_hlayout = QtWidgets.QHBoxLayout()
        self.show_peaks_hlayout.setObjectName("horizontalLayout3")

        self.show_matches_hlayout = QtWidgets.QHBoxLayout()
        self.show_matches_hlayout.setObjectName("horizontalLayout4")

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")

        self.vertical_layout.addItem(self.select_workspace_hlayout)
        self.vertical_layout.addItem(self.find_peaks_parameter_hlayout)
        self.vertical_layout.addItem(self.show_peaks_hlayout)
        self.vertical_layout.addWidget(self.match_table)
        self.vertical_layout.addItem(self.show_matches_hlayout)


        self.setLayout(self.vertical_layout)

    def setup_horizontal_layouts(self):
        self.find_peaks_button = QtWidgets.QPushButton("Find Peaks" , self)
        self.find_peaks_combobox = QtWidgets.QComboBox(self)

        self.select_workspace_hlayout.addWidget(self.find_peaks_button)
        self.select_workspace_hlayout.addWidget(self.find_peaks_combobox)

        self.min_energy_label = QtWidgets.QLabel("Min Energy (KeV) " , self)
        self.min_energy_line_edit = QtWidgets.QLineEdit( self)

        self.max_energy_label = QtWidgets.QLabel(" Max Energy (KeV) ", self)
        self.max_energy_line_edit = QtWidgets.QLineEdit(self)

        self.plot_peaks_label = QtWidgets.QLabel(" Plot Peaks ",self)
        self.plot_peaks_checkbox = QtWidgets.QCheckBox("" , self)

        self.threshold_label = QtWidgets.QLabel(" Acceptance Threshold ", self)
        self.threshold_line_edit = QtWidgets.QLineEdit( self)

        self.find_peaks_parameter_hlayout.addWidget(self.min_energy_label)
        self.find_peaks_parameter_hlayout.addWidget(self.min_energy_line_edit)
        self.find_peaks_parameter_hlayout.addWidget(self.max_energy_label)
        self.find_peaks_parameter_hlayout.addWidget(self.max_energy_line_edit)
        self.find_peaks_parameter_hlayout.addWidget(self.plot_peaks_label)
        self.find_peaks_parameter_hlayout.addWidget(self.plot_peaks_checkbox)
        self.find_peaks_parameter_hlayout.addWidget(self.threshold_label)
        self.find_peaks_parameter_hlayout.addWidget(self.threshold_line_edit)

        self.show_peaks_table_combobox = QtWidgets.QComboBox(self)
        self.show_peaks_table_button = QtWidgets.QPushButton(" Show peaks " , self)

        self.show_peaks_hlayout.addWidget(self.show_peaks_table_combobox)
        self.show_peaks_hlayout.addWidget(self.show_peaks_table_button)

        self.show_match_table_combobox = QtWidgets.QComboBox(self)
        self.show_match_table_button = QtWidgets.QPushButton(" Show matches ", self)

        self.show_matches_hlayout.addWidget(self.show_match_table_combobox)
        self.show_matches_hlayout.addWidget(self.show_match_table_button)

    def setup_buttons(self):
        self.find_peaks_button.clicked.connect(self.find_peaks_notifier.notify_subscribers)
        self.show_peaks_table_button.clicked.connect(self.show_peaks_table_notifier.notify_subscribers)
        self.show_match_table_button.clicked.connect(self.show_match_table_notifier.notify_subscribers)

    def get_parameters_for_find_peaks(self):
        parameters = {}
        try:
            parameters["min_energy"] = float(self.min_energy_line_edit.text())
            parameters["max_energy"] = float(self.max_energy_line_edit.text())
            parameters["threshold"] = float(self.threshold_line_edit.text())

        except ValueError:
            warning("Invalid arguments for peak finding")
            return None

        parameters["workspace"] = self.find_peaks_combobox.currentText()
        parameters["plot_peaks"] = self.plot_peaks_checkbox.isChecked()

        return parameters

    def add_options_to_find_peak_combobox(self , options):
        self.find_peaks_combobox.clear()
        self.find_peaks_combobox.addItems(options)

    def add_options_to_show_peak_combobox(self , options):
        self.show_peaks_table_combobox.clear()
        self.show_peaks_table_combobox.addItems(options)

    def add_options_to_show_matches_combobox(self , options):
        self.show_match_table_combobox.clear()
        self.show_match_table_combobox.addItems(options)

    def setup_intial_parameters(self):
        self.min_energy_line_edit.setText("50")
        self.max_energy_line_edit.setText("1000")
        self.threshold_line_edit.setText("20")

    def enable(self):
        self.setEnabled(True)

    def disable(self):
        self.setEnabled(False)