# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package
from qtpy.QtWidgets import QInputDialog


def input_qinputdialog(prompt: str = "") -> str:
    """
    Raises a QInputDialog with a given prompt and returns the user input as a string.
    If the user cancels the dialog, an EOFError is raised.
    Intended to be used to override python's `input` function to be more user friendly.
    """
    dlg = QInputDialog()
    dlg.setInputMode(QInputDialog.TextInput)
    dlg.setLabelText(str(prompt) if prompt is not None else "")
    accepted = dlg.exec_()
    if accepted:
        return dlg.textValue()
    else:
        raise EOFError("User input request cancelled")
