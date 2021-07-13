# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "general_fitting_options.ui")

MA_FIT_BY_OPTIONS = ["Run", "Group/Pair"]


class GeneralFittingOptionsView(ui_form, base_widget):
    """
    The GeneralFittingOptionsView includes the Simultaneous fitting options, and the cyclic dataset display combobox.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the GeneralFittingOptionsView. By default the simultaneous options are disabled."""
        super(GeneralFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.disable_simultaneous_fit_options()

        self._setup_simultaneous_fit_by_combo_box(MA_FIT_BY_OPTIONS)

    def set_slot_for_fitting_mode_changed(self, slot) -> None:
        """Connect the slot for the simultaneous fit check box."""
        self.simul_fit_checkbox.toggled.connect(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot) -> None:
        """Connect the slot for the fit by combo box being changed."""
        self.simul_fit_by_combo.currentIndexChanged.connect(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot) -> None:
        """Connect the slot for the fit specifier combo box being changed."""
        self.simul_fit_by_specifier.currentIndexChanged.connect(slot)

    def _setup_simultaneous_fit_by_combo_box(self, fit_by_options: list) -> None:
        """Populate the simultaneous fit by combo box."""
        for item in fit_by_options:
            self.simul_fit_by_combo.addItem(item)

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

    def setup_fit_by_specifier(self, choices: list) -> None:
        """Setup the fit by specifier combo box."""
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)
