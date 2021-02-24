# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator

from qtpy.QtWidgets import QWidget

ui_tf_asymmetry_fitting_options, _ = load_ui(__file__, "tf_asymmetry_fitting_options.ui")


class TFAsymmetryFittingOptionsView(QWidget, ui_tf_asymmetry_fitting_options):
    """
    The TFAsymmetryFittingOptionsView has a line edit to display the normalisation factor option.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the TFAsymmetryFittingOptionsView."""
        super(TFAsymmetryFittingOptionsView, self).__init__(parent)
        self.setupUi(self)

        self.normalisation_validator = LineEditDoubleValidator(self.normalisation_line_edit, 0.0)
        self.normalisation_line_edit.setValidator(self.normalisation_validator)

        self.tf_asymmetry_mode = False

    def set_slot_for_normalisation_changed(self, slot):
        self.normalisation_line_edit.editingFinished.connect(slot)

    @property
    def tf_asymmetry_mode(self):
        return not self.normalisation_line_edit.isHidden()

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on):
        if tf_asymmetry_on:
            self.show_normalisation_options()
        else:
            self.hide_normalisation_options()

    @property
    def normalisation(self):
        return float(self.normalisation_line_edit.text())

    @normalisation.setter
    def normalisation(self, value):
        self.normalisation_line_edit.blockSignals(True)
        self.normalisation_line_edit.setText(str(value))
        self.normalisation_line_edit.blockSignals(False)

    def hide_normalisation_options(self) -> None:
        """Hides the normalisation options."""
        self.normalisation_label.hide()
        self.normalisation_line_edit.hide()

    def show_normalisation_options(self) -> None:
        """Shows the normalisation options."""
        self.normalisation_label.show()
        self.normalisation_line_edit.show()
