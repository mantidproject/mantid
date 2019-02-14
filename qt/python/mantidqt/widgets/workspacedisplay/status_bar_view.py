# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtWidgets import QMainWindow, QStatusBar

from mantidqt.widgets.observers.observing_view import ObservingView


class StatusBarView(QMainWindow, ObservingView):
    close_signal = Signal()
    rename_signal = Signal(str)

    def __init__(self, parent, central_widget, name, window_width=600, window_height=400, presenter=None):
        super(StatusBarView, self).__init__(parent)

        self.presenter = presenter
        self.setCentralWidget(central_widget)
        self.setWindowTitle("{} - Mantid".format(name))
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.status_bar = QStatusBar(self)
        self.setStatusBar(self.status_bar)

        self.resize(window_width, window_height)
        self.close_signal.connect(self._run_close)
        self.rename_signal.connect(self._run_rename)

    def closeEvent(self, event):
        self.presenter.clear_observer()
        self.centralWidget().close()
        QMainWindow.closeEvent(self, event)
        self.deleteLater()

    @Slot()
    def _run_close(self):
        self.close()

    @Slot(str)
    def _run_rename(self, new_name):
        self._rename(new_name)
