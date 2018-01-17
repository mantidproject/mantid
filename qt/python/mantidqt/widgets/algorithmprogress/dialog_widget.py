from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (QDialog, QHeaderView, QHBoxLayout, QProgressBar, QPushButton,
                            QTreeWidget, QTreeWidgetItem, QVBoxLayout)

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
        self.close_button = QPushButton('Close')
        button_layout.addStretch()
        button_layout.addWidget(self.close_button)

        layout = QVBoxLayout()
        layout.addWidget(self.tree)
        layout.addLayout(button_layout)
        self.setLayout(layout)

        self.setWindowTitle('Mantid - Algorithm progress')
        self.setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"))
        self.resize(500, 300)

        self.presenter = AlgorithmProgressDialogPresenter(self, model)
        self.presenter.update()

    def update(self, data):
        """
        Update the gui elements.
        :param data: Data in format of AlgorithmProgressModel.get_running_algorithm_data()
        """
        self.tree.clear()
        for alg in data:
            item = QTreeWidgetItem([alg[0]])
            self.tree.addTopLevelItem(item)
            progress_bar = QProgressBar()
            progress_bar.setAlignment(Qt.AlignHCenter)
            cancel_button = QPushButton("Cancel")
            self.tree.setItemWidget(item, 1, progress_bar)
            self.tree.setItemWidget(item, 2, cancel_button)
            for prop in alg[1]:
                item.addChild(QTreeWidgetItem(prop))

