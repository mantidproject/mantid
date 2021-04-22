# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QWidget

ui_general_fitting_options, _ = load_ui(__file__, "general_fitting_options.ui")

MA_FIT_BY_OPTIONS = ["Run", "Group/Pair"]
SINGLE_FIT_LABEL = "Select Workspace"
SIMULTANEOUS_FIT_LABEL = "Display parameters for"


class GeneralFittingOptionsView(QWidget, ui_general_fitting_options):
    """
    The GeneralFittingOptionsView includes the Simultaneous fitting options, and the cyclic dataset display combobox.
    """

    def __init__(self, parent: QWidget = None, is_frequency_domain: bool = False):
        """Initializes the GeneralFittingOptionsView. By default the simultaneous options are disabled."""
        super(GeneralFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.increment_parameter_display_button.clicked.connect(self.increment_dataset_name_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_dataset_name_combo_box)

        self.disable_simultaneous_fit_options()

        if is_frequency_domain:
            self.hide_simultaneous_fit_options()
        else:
            self._setup_simultaneous_fit_by_combo_box(MA_FIT_BY_OPTIONS)

    def set_slot_for_dataset_changed(self, slot) -> None:
        """Connect the slot for the display workspace combo box being changed."""
        self.dataset_name_combo_box.currentIndexChanged.connect(slot)

    def set_slot_for_fitting_mode_changed(self, slot) -> None:
        """Connect the slot for the simultaneous fit check box."""
        self.simul_fit_checkbox.toggled.connect(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot) -> None:
        """Connect the slot for the fit by combo box being changed."""
        self.simul_fit_by_combo.currentIndexChanged.connect(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot) -> None:
        """Connect the slot for the fit specifier combo box being changed."""
        self.simul_fit_by_specifier.currentIndexChanged.connect(slot)

    def update_dataset_name_combo_box(self, dataset_names: list) -> None:
        """Update the data in the parameter display combo box."""
        old_name = self.dataset_name_combo_box.currentText()

        self.update_dataset_names_combo_box(dataset_names)

        new_index = self.dataset_name_combo_box.findText(old_name)
        new_index = new_index if new_index != -1 else 0

        self.dataset_name_combo_box.setCurrentIndex(new_index)
        # This signal isn't always sent, so I will emit it manually.
        self.dataset_name_combo_box.currentIndexChanged.emit(new_index)

    def update_dataset_names_combo_box(self, dataset_names: list) -> None:
        """Update the datasets displayed in the dataset name combobox."""
        self.dataset_name_combo_box.blockSignals(True)
        self.dataset_name_combo_box.clear()
        self.dataset_name_combo_box.addItems(dataset_names)
        self.dataset_name_combo_box.blockSignals(False)

    def increment_dataset_name_combo_box(self) -> None:
        """Increment the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index < count - 1:
            self.dataset_name_combo_box.setCurrentIndex(index + 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(0)

    def decrement_dataset_name_combo_box(self) -> None:
        """Decrement the parameter display combo box."""
        index = self.dataset_name_combo_box.currentIndex()
        count = self.dataset_name_combo_box.count()

        if index != 0:
            self.dataset_name_combo_box.setCurrentIndex(index - 1)
        else:
            self.dataset_name_combo_box.setCurrentIndex(count - 1)

    def _setup_simultaneous_fit_by_combo_box(self, fit_by_options: list) -> None:
        """Populate the simultaneous fit by combo box."""
        for item in fit_by_options:
            self.simul_fit_by_combo.addItem(item)

    @property
    def current_dataset_name(self) -> str:
        """Returns the selected dataset name."""
        return str(self.dataset_name_combo_box.currentText())

    @current_dataset_name.setter
    def current_dataset_name(self, dataset_name: str) -> None:
        """Sets the currently selected dataset name."""
        index = self.dataset_name_combo_box.findText(dataset_name)
        if index != -1:
            self.dataset_name_combo_box.setCurrentIndex(index)

    def number_of_datasets(self) -> int:
        """Returns the number of dataset names loaded into the widget."""
        return self.dataset_name_combo_box.count()

    @property
    def simultaneous_fit_by(self) -> str:
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.simul_fit_by_combo.currentText()

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text: str) -> None:
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        index = self.simul_fit_by_combo.findText(fit_by_text)
        if index != -1:
            self.simul_fit_by_combo.blockSignals(True)
            self.simul_fit_by_combo.setCurrentIndex(index)
            self.simul_fit_by_combo.blockSignals(False)

    @property
    def simultaneous_fit_by_specifier(self) -> str:
        """Returns the run, group or pair name."""
        return self.simul_fit_by_specifier.currentText()

    @property
    def current_dataset_index(self) -> int:
        """Returns the index of the currently displayed dataset."""
        current_index = self.dataset_name_combo_box.currentIndex()
        return current_index if current_index != -1 else None

    def switch_to_simultaneous(self) -> None:
        """Switches the view to simultaneous fit mode."""
        self.set_workspace_combo_box_label(SIMULTANEOUS_FIT_LABEL)
        self.enable_simultaneous_fit_options()

    def switch_to_single(self) -> None:
        """Switches the view to single fit mode."""
        self.set_workspace_combo_box_label(SINGLE_FIT_LABEL)
        self.disable_simultaneous_fit_options()

    def hide_simultaneous_fit_options(self) -> None:
        """Hides the simultaneous fit options."""
        self.simul_fit_checkbox.hide()
        self.simul_fit_by_combo.hide()
        self.simul_fit_by_specifier.hide()

    def disable_simultaneous_fit_options(self) -> None:
        """Disables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(False)
        self.simul_fit_by_specifier.setEnabled(False)

    def enable_simultaneous_fit_options(self) -> None:
        """Enables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(True)
        self.simul_fit_by_specifier.setEnabled(True)

    @property
    def simultaneous_fitting_mode(self) -> bool:
        """Returns true if in simultaneous mode."""
        return self.simul_fit_checkbox.isChecked()

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, simultaneous: bool) -> None:
        """Sets whether or not you are in simultaneous mode."""
        self.simul_fit_checkbox.blockSignals(True)
        self.simul_fit_checkbox.setChecked(simultaneous)
        self.simul_fit_checkbox.blockSignals(False)

    def set_workspace_combo_box_label(self, text: str) -> None:
        self.workspace_combo_box_label.setText(text)

    def setup_fit_by_specifier(self, choices: list) -> None:
        """Setup the fit by specifier combo box."""
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)
