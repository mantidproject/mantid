from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget, QHBoxLayout, QProgressBar, QPushButton

from .dialog_widget import AlgorithmMonitorDialog
from .presenter import AlgorithmProgressPresenter


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

    def show_progress_bar(self):
        if self.progress_bar is None:
            self.progress_bar = QProgressBar()
            self.progress_bar.setAlignment(Qt.AlignHCenter)
            self.layout.insertWidget(0, self.progress_bar)
            self.layout.removeItem(self.layout.takeAt(1))

    def hide_progress_bar(self):
        if self.progress_bar is not None:
            self.layout.insertStretch(0)
            self.layout.removeWidget(self.progress_bar)
            self.progress_bar.close()
            self.progress_bar = None

    def show_dialog(self):
        dialog = AlgorithmMonitorDialog(self, self.presenter.model)
        dialog.show()
