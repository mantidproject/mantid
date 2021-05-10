# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.Common import message_box


MIN_ENERGY = "50"
MAX_ENERGY = "1000"
ACCEPTANCE_THRESHOLD = "0.01"
MIN_WIDTH = "0.5"
MAX_WIDTH = "30"
ESTIMATE_WIDTH = "3"
USE_DEFAULT_PEAK_WIDTH = True


class EAAutoTabView(QtWidgets.QWidget):

    def __init__(self, match_table, parent=None):
        super(EAAutoTabView, self).__init__(parent)

        self.match_table = match_table
        self.find_peaks_notifier = GenericObservable()
        self.show_peaks_table_notifier = GenericObservable()
        self.show_match_table_notifier = GenericObservable()
        self.clear_match_table_notifier = GenericObservable()

        self.setup_interface_layout()
        self.setup_horizontal_layouts()
        self.setup_buttons()
        self.setup_intial_parameters()

    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.peak_info_label = QtWidgets.QLabel(self)
        self.peak_info_label.setVisible(False)

        self.select_workspace_hlayout = QtWidgets.QHBoxLayout()
        self.select_workspace_hlayout.setObjectName("horizontalLayout1")

        self.find_peaks_parameter_hlayout1 = QtWidgets.QHBoxLayout()
        self.find_peaks_parameter_hlayout1.setObjectName("horizontalLayout2")

        self.find_peaks_parameter_hlayout2 = QtWidgets.QHBoxLayout()
        self.find_peaks_parameter_hlayout2.setObjectName("horizontalLayout3")

        self.find_peaks_parameter_hlayout3 = QtWidgets.QHBoxLayout()
        self.find_peaks_parameter_hlayout3.setObjectName("horizontalLayout4")

        self.show_peaks_hlayout = QtWidgets.QHBoxLayout()
        self.show_peaks_hlayout.setObjectName("horizontalLayout5")

        self.show_matches_hlayout = QtWidgets.QHBoxLayout()
        self.show_matches_hlayout.setObjectName("horizontalLayout6")

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")

        self.clear_match_table_button = QtWidgets.QPushButton("Clear Table", self)
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.clear_match_table_button.sizePolicy().hasHeightForWidth())
        self.clear_match_table_button.setSizePolicy(size_policy)
        self.clear_match_table_button.setToolTip("Removes all rows in table")

        self.vertical_layout.addItem(self.select_workspace_hlayout)
        self.vertical_layout.addItem(self.find_peaks_parameter_hlayout1)
        self.vertical_layout.addItem(self.find_peaks_parameter_hlayout2)
        self.vertical_layout.addItem(self.find_peaks_parameter_hlayout3)
        self.vertical_layout.addWidget(self.peak_info_label)
        self.vertical_layout.addItem(self.show_peaks_hlayout)
        self.vertical_layout.addWidget(self.match_table)
        self.vertical_layout.addWidget(self.clear_match_table_button)
        self.vertical_layout.addItem(self.show_matches_hlayout)

        self.setLayout(self.vertical_layout)

    def setup_horizontal_layouts(self):
        self.find_peaks_button = QtWidgets.QPushButton("Find Peaks", self)
        self.find_peaks_combobox = QtWidgets.QComboBox(self)

        self.select_workspace_hlayout.addWidget(self.find_peaks_button)
        self.select_workspace_hlayout.addWidget(self.find_peaks_combobox)

        self.min_energy_label = QtWidgets.QLabel("Minimum Energy (KeV) ", self)
        self.min_energy_line_edit = QtWidgets.QLineEdit(self)

        self.max_energy_label = QtWidgets.QLabel(" Maximum Energy (KeV) ", self)
        self.max_energy_line_edit = QtWidgets.QLineEdit(self)

        self.plot_peaks_label = QtWidgets.QLabel(" Plot Peaks ", self)
        self.plot_peaks_checkbox = QtWidgets.QCheckBox("", self)

        self.threshold_label = QtWidgets.QLabel(" Acceptance Threshold ", self)
        self.threshold_line_edit = QtWidgets.QLineEdit(self)

        self.find_peaks_parameter_hlayout1.addWidget(self.min_energy_label)
        self.find_peaks_parameter_hlayout1.addWidget(self.min_energy_line_edit)
        self.find_peaks_parameter_hlayout1.addWidget(self.max_energy_label)
        self.find_peaks_parameter_hlayout1.addWidget(self.max_energy_line_edit)
        self.find_peaks_parameter_hlayout1.addWidget(self.plot_peaks_label)
        self.find_peaks_parameter_hlayout1.addWidget(self.plot_peaks_checkbox)
        self.find_peaks_parameter_hlayout1.addWidget(self.threshold_label)
        self.find_peaks_parameter_hlayout1.addWidget(self.threshold_line_edit)

        self.default_peak_label = QtWidgets.QLabel(" Use default peak widths ? ", self)
        self.default_peak_checkbox = QtWidgets.QCheckBox(self)
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        self.default_peak_label.setSizePolicy(size_policy)
        self.default_peak_checkbox.setSizePolicy(size_policy)

        self.find_peaks_parameter_hlayout2.addWidget(self.default_peak_checkbox)
        self.find_peaks_parameter_hlayout2.addWidget(self.default_peak_label)

        self.min_width_label = QtWidgets.QLabel("Minimum peak width (KeV) ", self)
        self.min_width_line_edit = QtWidgets.QLineEdit(self)

        self.max_width_label = QtWidgets.QLabel(" Maximum peak width (KeV) ", self)
        self.max_width_line_edit = QtWidgets.QLineEdit(self)

        self.estimate_width_label = QtWidgets.QLabel(" Estimate peak width (KeV) ", self)
        self.estimate_width_line_edit = QtWidgets.QLineEdit(self)

        self.find_peaks_parameter_hlayout3.addWidget(self.min_width_label)
        self.find_peaks_parameter_hlayout3.addWidget(self.min_width_line_edit)
        self.find_peaks_parameter_hlayout3.addWidget(self.max_width_label)
        self.find_peaks_parameter_hlayout3.addWidget(self.max_width_line_edit)
        self.find_peaks_parameter_hlayout3.addWidget(self.estimate_width_label)
        self.find_peaks_parameter_hlayout3.addWidget(self.estimate_width_line_edit)

        self.show_peaks_table_combobox = QtWidgets.QComboBox(self)
        self.show_peaks_table_button = QtWidgets.QPushButton(" Show peaks ", self)

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
        self.clear_match_table_button.clicked.connect(self.clear_match_table_notifier.notify_subscribers)
        self.default_peak_checkbox.clicked.connect(self.default_peak_checkbox_changed)

    def get_parameters_for_find_peaks(self):
        parameters = {}
        try:
            parameters["min_energy"] = float(self.min_energy_line_edit.text())
            parameters["max_energy"] = float(self.max_energy_line_edit.text())
            parameters["threshold"] = float(self.threshold_line_edit.text())
            parameters["default_width"] = self.default_peak_checkbox.isChecked()
            if not parameters["default_width"]:
                parameters["min_width"] = float(self.min_width_line_edit.text())
                parameters["max_width"] = float(self.max_width_line_edit.text())
                parameters["estimate_width"] = float(self.estimate_width_line_edit.text())
                if parameters["min_width"] > parameters["max_width"]:
                    raise ValueError("Minimum peak width is greater than maximum peak width")
                if parameters["min_width"] > parameters["estimate_width"] or \
                        parameters["estimate_width"] > parameters["max_width"]:
                    raise ValueError("Estimated peak width must be between minimum and maximum peak width")
            if parameters["min_energy"] > parameters["max_energy"]:
                raise ValueError("Minimum energy is greater than maximum energy")

        except ValueError as error:
            message_box.warning(f"Invalid arguments for peak finding: {error}")
            return None
        workspace_name = self.find_peaks_combobox.currentText()
        if workspace_name == "":
            message_box.warning("No workspace selected")
            return None
        parameters["workspace"] = workspace_name
        parameters["plot_peaks"] = self.plot_peaks_checkbox.isChecked()

        return parameters

    def add_options_to_find_peak_combobox(self, options):
        self.find_peaks_combobox.clear()
        self.find_peaks_combobox.addItems(options)

    def add_options_to_show_peak_combobox(self, options):
        self.show_peaks_table_combobox.clear()
        self.show_peaks_table_combobox.addItems(options)

    def add_options_to_show_matches_combobox(self, options):
        self.show_match_table_combobox.clear()
        self.show_match_table_combobox.addItems(options)

    def setup_intial_parameters(self):
        self.min_energy_line_edit.setText(MIN_ENERGY)
        self.max_energy_line_edit.setText(MAX_ENERGY)
        self.threshold_line_edit.setText(ACCEPTANCE_THRESHOLD)
        self.min_width_line_edit.setText(MIN_WIDTH)
        self.max_width_line_edit.setText(MAX_WIDTH)
        self.estimate_width_line_edit.setText(ESTIMATE_WIDTH)
        self.default_peak_checkbox.setChecked(USE_DEFAULT_PEAK_WIDTH)
        self.set_peak_width_visible(False)

    def set_peak_width_visible(self, arg):
        self.min_width_label.setVisible(arg)
        self.min_width_line_edit.setVisible(arg)
        self.max_width_label.setVisible(arg)
        self.max_width_line_edit.setVisible(arg)
        self.estimate_width_label.setVisible(arg)
        self.estimate_width_line_edit.setVisible(arg)

    def enable(self):
        self.setEnabled(True)

    def disable(self):
        self.setEnabled(False)

    def set_peak_info(self, workspace, number_of_peaks):
        label_text = f"{number_of_peaks} peak(s) found in {workspace}"
        self.peak_info_label.setText(label_text)
        self.peak_info_label.setVisible(True)

    def default_peak_checkbox_changed(self):
        if not self.default_peak_checkbox.isChecked():
            self.set_peak_width_visible(True)
        else:
            self.set_peak_width_visible(False)
