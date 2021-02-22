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
        self.normalisation_validator.setBottom(0.0)
        self.normalisation_line_edit.setValidator(self.normalisation_validator)

        self.hide_normalisation_options()

    def hide_normalisation_options(self) -> None:
        """Hides the normalisation options."""
        self.normalisation_label.hide()
        self.normalisation_line_edit.hide()

    def show_normalisation_options(self) -> None:
        """Shows the normalisation options."""
        self.normalisation_label.show()
        self.normalisation_line_edit.show()
