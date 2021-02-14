# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy import QtWidgets

from mantid import logger

ui_general_fitting_options, _ = load_ui(__file__, "general_fitting_options.ui")

FDA_FITTING_OPTIONS = []
MA_FITTING_OPTIONS = ["Run", "Group/Pair"]


class GeneralFittingOptionsView(QtWidgets.QWidget, ui_general_fitting_options):

    def __init__(self, parent=None, is_frequency_domain=False):
        super(GeneralFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.increment_parameter_display_button.clicked.connect(self.increment_dataset_name_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_dataset_name_combo_box)

        self.disable_simultaneous_fit_options()

        if is_frequency_domain:
            self._setup_simultaneous_fit_by_combo_box(FDA_FITTING_OPTIONS)
            self.hide_simultaneous_fit_options()
        else:
            self._setup_simultaneous_fit_by_combo_box(MA_FITTING_OPTIONS)

    def set_slot_for_dataset_changed(self, slot):
        """Connect the slot for the display workspace combo box being changed."""
        self.dataset_name_combo_box.currentIndexChanged.connect(slot)

    def set_slot_for_fitting_mode_changed(self, slot):
        """Connect the slot for the simultaneous fit check box."""
        self.simul_fit_checkbox.toggled.connect(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot):
        """Connect the slot for the fit by combo box being changed."""
        self.simul_fit_by_combo.currentIndexChanged.connect(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot):
        """Connect the slot for the fit specifier combo box being changed."""
        self.simul_fit_by_specifier.currentIndexChanged.connect(slot)

    def update_dataset_name_combo_box(self, dataset_names):
        """Update the data in the parameter display combo box."""
        old_name = self.dataset_name_combo_box.currentText()

        self.update_dataset_names_combo_box(dataset_names)

        new_index = self.dataset_name_combo_box.findText(old_name)
        new_index = new_index if new_index != -1 else 0

        self.dataset_name_combo_box.setCurrentIndex(new_index)
        # This signal isn't always sent, so I will emit it manually.
        self.dataset_name_combo_box.currentIndexChanged.emit(new_index)

    def update_dataset_names_combo_box(self, dataset_names):
        """Update the datasets displayed in the dataset name combobox."""
        self.dataset_name_combo_box.blockSignals(True)
        self.dataset_name_combo_box.clear()
        self.dataset_name_combo_box.addItems(dataset_names)
        self.dataset_name_combo_box.blockSignals(False)

    def increment_dataset_name_combo_box(self):
        """Increment the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index < count - 1:
            self.dataset_name_combo_box.setCurrentIndex(index + 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(0)

    def decrement_dataset_name_combo_box(self):
        """Decrement the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index != 0:
            self.dataset_name_combo_box.setCurrentIndex(index - 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(count - 1)

    def _setup_simultaneous_fit_by_combo_box(self, item_list):
        """Populate the simultaneous fit by combo box."""
        for item in item_list:
            self.simul_fit_by_combo.addItem(item)

    @property
    def current_dataset_name(self):
        """Returns the selected dataset name."""
        return str(self.dataset_name_combo_box.currentText())

    @current_dataset_name.setter
    def current_dataset_name(self, dataset_name):
        """Sets the currently selected dataset name."""
        index = self.dataset_name_combo_box.findText(dataset_name)
        if index != -1:
            self.dataset_name_combo_box.setCurrentIndex(index)

    def number_of_datasets(self):
        return self.dataset_name_combo_box.count()

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

    @property
    def current_dataset_index(self):
        """Returns the index of the currently displayed dataset."""
        current_index = self.dataset_name_combo_box.currentIndex()
        return current_index if current_index != -1 else None

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
