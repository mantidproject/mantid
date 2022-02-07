# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "tf_asymmetry_fitting_options.ui")


class TFAsymmetryFittingOptionsView(ui_form, base_widget):
    """
    The TFAsymmetryFittingOptionsView has a line edit to display the normalisation factor option.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the TFAsymmetryFittingOptionsView."""
        super(TFAsymmetryFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.normalisation_validator = LineEditDoubleValidator(self.normalisation_line_edit, 0.0)
        self.normalisation_line_edit.setValidator(self.normalisation_validator)

    def set_slot_for_normalisation_changed(self, slot) -> None:
        """Sets the slot for handling when a normalisation value is changed by the user."""
        self.normalisation_line_edit.editingFinished.connect(slot)

    def set_slot_for_fix_normalisation_changed(self, slot) -> None:
        """Sets the slot for handling when a fix normalisation is ticked or un-ticked by the user."""
        self.fix_normalisation_checkbox.stateChanged.connect(slot)

    def set_tf_asymmetry_mode(self, tf_asymmetry_on: bool) -> None:
        """Hides or shows the normalisation options depending on if TF Asymmetry fitting mode is on or off."""
        if tf_asymmetry_on:
            self.show_normalisation_options()
        else:
            self.hide_normalisation_options()

    @property
    def normalisation(self) -> float:
        """Returns the normalisation value currently displayed in the normalisation line edit."""
        return float(self.normalisation_line_edit.text())

    def set_normalisation(self, value: float, error: float) -> None:
        """Sets the normalisation value currently displayed in the normalisation line edit."""
        self.normalisation_line_edit.blockSignals(True)
        self.normalisation_line_edit.setText(f"{value:.5f}")
        self.normalisation_error_line_edit.setText(f"({error:.5f})")
        self.normalisation_line_edit.blockSignals(False)

    @property
    def is_normalisation_fixed(self) -> bool:
        """Returns true if the fix normalisation check box is ticked."""
        return self.fix_normalisation_checkbox.isChecked()

    @is_normalisation_fixed.setter
    def is_normalisation_fixed(self, is_fixed: bool) -> None:
        """Sets whether the fix normalisation checkbox is ticked or not."""
        self.fix_normalisation_checkbox.blockSignals(True)
        self.fix_normalisation_checkbox.setChecked(is_fixed)
        self.fix_normalisation_checkbox.blockSignals(False)

    def hide_normalisation_options(self) -> None:
        """Hides the normalisation options."""
        self.fix_normalisation_checkbox.hide()
        self.normalisation_line_edit.hide()
        self.normalisation_error_line_edit.hide()

    def show_normalisation_options(self) -> None:
        """Shows the normalisation options."""
        self.fix_normalisation_checkbox.show()
        self.normalisation_line_edit.show()
        self.normalisation_error_line_edit.show()
