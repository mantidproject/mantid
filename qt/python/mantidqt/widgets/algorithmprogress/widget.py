# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QHBoxLayout, QProgressBar, QPushButton, QWidget

from mantidqt.utils.qt import import_qt
from mantidqt.widgets.algorithmprogress.dialog_widget import AlgorithmMonitorDialog
from mantidqt.widgets.algorithmprogress.presenter import AlgorithmProgressPresenter


class AlgorithmProgressWidget(QWidget):
    """
    Widget consisting of a progress bar and a button.
    """

    def __init__(self, parent=None):
        super(AlgorithmProgressWidget, self).__init__(parent)
        self.progress_bar = None
        self.details_button = QPushButton('Details')
        self.details_button.clicked.connect(self.show_dialog)
        self.layout = QHBoxLayout()
        self.layout.addStretch()
        self.layout.addWidget(self.details_button)
        self.setLayout(self.layout)
        self.presenter = AlgorithmProgressPresenter(self)
        self.algorithm_monitor_dialog = None

    def show_progress_bar(self):
        if self.progress_bar is None:
            self.progress_bar = QProgressBar()
            self.progress_bar.setAttribute(Qt.WA_DeleteOnClose, True)
            self.progress_bar.setAlignment(Qt.AlignHCenter)
            self.layout.insertWidget(0, self.progress_bar)
            self.layout.removeItem(self.layout.takeAt(1))

    def hide_progress_bar(self):
        if self.progress_bar is not None:
            self.layout.insertStretch(0)
            self.layout.removeWidget(self.progress_bar)
            self.progress_bar.close()
            self.progress_bar = None
            # self.progress_bar.reset()

    def show_dialog(self):
        if not self.algorithm_monitor_dialog:
            self.algorithm_monitor_dialog = AlgorithmMonitorDialog(self, self.presenter.model)
            self.algorithm_monitor_dialog.show()
        else:
            self.algorithm_monitor_dialog.setFocus()

    def clear_dialog(self):
        self.algorithm_monitor_dialog = None
