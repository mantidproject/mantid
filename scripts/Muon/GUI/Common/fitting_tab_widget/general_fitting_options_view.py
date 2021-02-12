# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy import QtWidgets

ui_general_fitting_options, _ = load_ui(__file__, "general_fitting_options.ui")

FDA_FITTING_OPTIONS = []
MA_FITTING_OPTIONS = ["Run", "Group/Pair"]


class GeneralFittingOptionsView(QtWidgets.QWidget, ui_general_fitting_options):

    def __init__(self, parent=None, is_frequency_domain=False):
        super(GeneralFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.increment_parameter_display_button.clicked.connect(self.increment_display_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_display_combo_box)

        self.disable_simultaneous_fit_options()

        if is_frequency_domain:
            self._setup_simultaneous_fit_by_combo_box(FDA_FITTING_OPTIONS)
            self.hide_simultaneous_fit_options()
        else:
            self._setup_simultaneous_fit_by_combo_box(MA_FITTING_OPTIONS)

    def set_slot_for_display_workspace_changed(self, slot):
        """Connect the slot for the display workspace combo box being changed."""
        self.parameter_display_combo.currentIndexChanged.connect(slot)

    def set_slot_for_fitting_mode_changed(self, slot):
        """Connect the slot for the simultaneous fit check box."""
        self.simul_fit_checkbox.toggled.connect(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot):
        """Connect the slot for the fit by combo box being changed."""
        self.simul_fit_by_combo.currentIndexChanged.connect(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot):
        """Connect the slot for the fit specifier combo box being changed."""
        self.simul_fit_by_specifier.currentIndexChanged.connect(slot)

    def update_displayed_data_combo_box(self, data_list):
        """Update the data in the parameter display combo box."""
        self.parameter_display_combo.blockSignals(True)
        name = self.parameter_display_combo.currentText()
        self.parameter_display_combo.clear()
        self.parameter_display_combo.addItems(data_list)

        index = self.parameter_display_combo.findText(name)
        self.parameter_display_combo.blockSignals(False)

        if index != -1:
            self.parameter_display_combo.setCurrentIndex(index)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def increment_display_combo_box(self):
        """Increment the parameter display combo box."""
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index < count - 1:
            self.parameter_display_combo.setCurrentIndex(index + 1)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def decrement_display_combo_box(self):
        """Decrement the parameter display combo box."""
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index != 0:
            self.parameter_display_combo.setCurrentIndex(index - 1)
        else:
            self.parameter_display_combo.setCurrentIndex(count - 1)

    def _setup_simultaneous_fit_by_combo_box(self, item_list):
        """Populate the simultaneous fit by combo box."""
        for item in item_list:
            self.simul_fit_by_combo.addItem(item)

    @property
    def display_workspace(self):
        """Returns the name of the currently displayed workspace parameter."""
        return str(self.parameter_display_combo.currentText())

    @display_workspace.setter
    def display_workspace(self, value):
        """Sets the name of the currently displayed workspace parameter."""
        index = self.parameter_display_combo.findText(value)
        if index != -1:
            self.parameter_display_combo.blockSignals(True)
            self.parameter_display_combo.setCurrentIndex(index)
            self.parameter_display_combo.blockSignals(False)

    def number_of_domains(self):
        return self.parameter_display_combo.count()

    @property
    def simultaneous_fit_by(self):
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.simul_fit_by_combo.currentText()

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text):
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        index = self.simul_fit_by_combo.findText(fit_by_text)
        if index != -1:
            self.simul_fit_by_combo.blockSignals(True)
            self.simul_fit_by_combo.setCurrentIndex(index)
            self.simul_fit_by_combo.blockSignals(False)

    @property
    def simultaneous_fit_by_specifier(self):
        """Returns the run, group or pair name."""
        return self.simul_fit_by_specifier.currentText()

    def get_index_for_start_end_times(self):
        """Returns the index of the currently displayed workspace."""
        current_index = self.parameter_display_combo.currentIndex()
        return current_index if current_index != -1 else 0

    def hide_simultaneous_fit_options(self):
        """Hides the simultaneous fit options."""
        self.simul_fit_checkbox.hide()
        self.simul_fit_by_combo.hide()
        self.simul_fit_by_specifier.hide()

    def disable_simultaneous_fit_options(self):
        """Disables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(False)
        self.simul_fit_by_specifier.setEnabled(False)

    def enable_simultaneous_fit_options(self):
        """Enables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(True)
        self.simul_fit_by_specifier.setEnabled(True)

    @property
    def is_simultaneous_fit_ticked(self):
        """Returns true if in simultaneous mode."""
        return self.simul_fit_checkbox.isChecked()

    @is_simultaneous_fit_ticked.setter
    def is_simultaneous_fit_ticked(self, simultaneous):
        """Sets whether or not you are in simultaneous mode."""
        self.simul_fit_checkbox.blockSignals(True)
        self.simul_fit_checkbox.setChecked(simultaneous)
        self.simul_fit_checkbox.blockSignals(False)

    def setup_fit_by_specifier(self, choices):
        """Setup the fit by specifier combo box."""
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)

    def set_workspace_combo_box_label(self, text):
        """Sets the text in the workspace combo box label."""
        self.workspace_combo_box_label.setText(text)
