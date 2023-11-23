# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class for small input dialogs
"""
from pathlib import Path

from qtpy.QtWidgets import QDialog
from qtpy.uic import loadUi


class DNSDialog(QDialog):
    """class for dialogs which can be opend by the gui """
    def __init__(self, filen=None, ui=None):
        super().__init__()
        path = str(Path(filen).parent.absolute())
        self._content = loadUi(path + ui, self)
