# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QMessageBox

from mantid import logger


class WorkbenchErrorMessageBox(QMessageBox):
    def __init__(self, parent, exception_message):
        super(QMessageBox, self).__init__(parent)
        self.parent = parent
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.setText(
            """Sorry, Mantid Workbench has caught an unexpected exception:\n
{0}
Would you like to terminate Mantid Workbench or try to continue working?
If you choose to continue it is advisable to save your data and restart the application.""".format(exception_message)
        )

        self.terminate_button = self.addButton("Terminate", QMessageBox.ActionRole)
        self.continue_button = self.addButton("Continue", QMessageBox.ActionRole)
        self.setIcon(QMessageBox.Critical)
        self.buttonClicked.connect(self.action_button_clicked)

    def action_button_clicked(self, button):
        self.close()
        if button == self.terminate_button:
            logger.fatal("Terminated by user.")
            self.parent.close()
        else:
            logger.fatal("Continue working.")
