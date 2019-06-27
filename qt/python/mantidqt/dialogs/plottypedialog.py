# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog, QHBoxLayout, QPushButton, QSizePolicy


class PlotTypeDialog(QDialog):
    Colorfill = 1
    Spectra = 2

    def __init__(self, parent=None, *args, **kwargs):
        QDialog.__init__(self, parent, *args, **kwargs)
        self.icon = self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setWindowFlags(Qt.WindowSystemMenuHint | Qt.WindowTitleHint |
                            Qt.WindowCloseButtonHint)

        self.selection = None
        self.decision = None
        self.setWindowTitle("Select Plot Type")

        self.btn_spectra = QPushButton("Spectra")
        self.btn_colorfil = QPushButton("Colorfill")

        self.hl = QHBoxLayout()
        self.hl.addWidget(self.btn_spectra)
        self.hl.addWidget(self.btn_colorfil)

        self.setLayout(self.hl)
        self.setMinimumWidth(200)

        self.btn_spectra.clicked.connect(self.show_spectra_selection_dialog)
        self.btn_colorfil.clicked.connect(self.make_colorfill_plot)

    def make_colorfill_plot(self, _):
        self.decision = self.Colorfill
        self.accept()

    def show_spectra_selection_dialog(self):
        self.decision = self.Spectra
        self.accept()
