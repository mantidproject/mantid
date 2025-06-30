# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class for small input dialogs.
"""

from pathlib import Path

from qtpy.QtWidgets import QDialog
from qtpy.uic import loadUi


class DNSDialog(QDialog):
    """
    Class for dialogs which can be opened by the GUI.
    """

    def __init__(self, files=None, ui=None):
        super().__init__()
        path = str(Path(files).parent.absolute())
        self._content = loadUi(path + ui, self)
