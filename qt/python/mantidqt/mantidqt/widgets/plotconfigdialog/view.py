# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog, QMessageBox

from mantidqt.utils.qt import load_ui


class PlotConfigDialogView(QDialog):

    def __init__(self, parent=None):
        super(PlotConfigDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'plot_config_dialog.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setWindowModality(Qt.WindowModal)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.errors_label = self.ui.errors
        self.minimum_errors_text = self.errors_label.text()
        self.errors_label.hide()

    def add_tab_widget(self, tab_widget):
        self.main_tab_widget.addTab(*tab_widget)

    def set_current_tab_widget(self, tab_widget):
        self.main_tab_widget.setCurrentWidget(tab_widget)

    def set_error_text(self, text=None):
        if not text:
            self.errors_label.hide()
            return

        self.errors_label.setText(self.minimum_errors_text + text)
        self.errors_label.show()

    def closeEvent(self, event):
        if self.errors_label.isVisible():
            reply = QMessageBox().question(self, "Are you sure?",
                                           "There are errors applying plot settings, are you sure?",
                                           QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                event.accept()
            else:
                event.ignore()
