from functools import partial
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog, QHBoxLayout, QPushButton


class PlotTypeDialog(QDialog):
    Colorfill = 1
    Spectra = 2

    def __init__(self, parent=None, *args, **kwargs):
        QDialog.__init__(self, parent, *args, **kwargs)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.selection = None
        self.decision = None
        self.setWindowTitle("Select Plot Type")

        self.btn_spectra = QPushButton("Spectrum")
        self.btn_colorfil = QPushButton("Colorfill")

        self.hl = QHBoxLayout()
        self.hl.addWidget(self.btn_spectra)
        self.hl.addWidget(self.btn_colorfil)

        self.setLayout(self.hl)

        self.btn_spectra.clicked.connect(self.show_spectra_selection_dialog)
        self.btn_colorfil.clicked.connect(self.make_colorfill_plot)

    def make_colorfill_plot(self, _):
        self.decision = self.Colorfill
        self.accept()

    def show_spectra_selection_dialog(self):
        self.decision = self.Spectra
        self.accept()
