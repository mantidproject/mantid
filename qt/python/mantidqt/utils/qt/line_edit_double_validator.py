# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtGui import QDoubleValidator

from mantid import logger


class LineEditDoubleValidator(QDoubleValidator):
    """
    A class for validating a line edit for entering doubles in decimal format or e notation. It catches invalid input
    such as empty strings and broken e notation, and then resets the value in the line edit to the last valid value.
    """
    def __init__(self, line_edit, initial_value):
        """Create a line edit double validator."""
        super(LineEditDoubleValidator, self).__init__(None)
        self.last_valid_value = initial_value
        self._line_edit = line_edit

        self._line_edit.editingFinished.connect(self.on_editing_finished)

    def on_editing_finished(self):
        """Entered when the data input is valid according to QDoubleValidator."""
        self.last_valid_value = self._line_edit.text()

    def fixup(self, new_value):
        """Entered when the data input is invalid according to QDoubleValidator (empty string or broken e notation)."""
        logger.warning(f"An invalid value '{new_value}' was provided. Using '{self.last_valid_value}' instead.")
        self._line_edit.setText(str(self.last_valid_value))
