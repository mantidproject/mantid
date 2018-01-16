from __future__ import absolute_import, print_function

from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (QDialog, QTreeWidget, QHeaderView, QHBoxLayout, QPushButton, QVBoxLayout)

from mantidqt import resources
from .dialog_presenter import AlgorithmProgressDialogPresenter


class AlgorithmMonitorDialog(QDialog):
    """
    Displays progress of all running algorithms.
    """
    def __init__(self, parent, model):
        super(AlgorithmMonitorDialog, self).__init__(parent)
        self.tree = QTreeWidget(self)
        self.tree.setColumnCount(3)
        self.tree.setSelectionMode(QTreeWidget.NoSelection)
        self.tree.setColumnWidth(0, 220)
        self.tree.setHeaderLabels(['Algorithm', 'Progress', ''])
        header = self.tree.header()
        header.setSectionResizeMode(1, QHeaderView.Stretch)
        header.setSectionResizeMode(2, QHeaderView.Fixed)
        header.setStretchLastSection(False)

        button_layout = QHBoxLayout()
        close_button = QPushButton('Close')
        close_button.clicked.connect(self.close)
        button_layout.addStretch()
        button_layout.addWidget(close_button)

        layout = QVBoxLayout()
        layout.addWidget(self.tree)
        layout.addLayout(button_layout)
        self.setLayout(layout)

        self.setWindowTitle('Mantid - Algorithm progress')
        self.setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"))
        self.resize(500, 300)

        self.presenter = AlgorithmProgressDialogPresenter(self, model)
        self.presenter.update()

