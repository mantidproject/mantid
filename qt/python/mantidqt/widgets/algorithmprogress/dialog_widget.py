from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (QDialog, QHeaderView, QHBoxLayout, QProgressBar, QPushButton,
                            QTreeWidget, QTreeWidgetItem, QVBoxLayout)

from mantidqt import resources  # noqa
from .dialog_presenter import AlgorithmProgressDialogPresenter


class CancelButton(QPushButton):
    """
    Button that cancels an algorithm
    """
    def __init__(self, presenter, algorithm_id):
        """
        Init an instance
        :param presenter: The presenter for the dialog
        :param algorithm_id: Some id of an algorithm
        """
        super(CancelButton, self).__init__('Cancel')
        self.presenter = presenter
        self.algorithm_id = algorithm_id
        self.clicked.connect(self.cancel_algorithm, Qt.QueuedConnection)

    def cancel_algorithm(self):
        self.presenter.cancel_algorithm(self.algorithm_id)


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
        self.presenter.update_gui()

    def update(self, data):
        """
        Update the gui elements.
        :param data: Data in format of AlgorithmProgressModel.get_running_algorithm_data()
        """
        self.tree.clear()
        for alg_data in data:
            name, id, properties = alg_data
            item = QTreeWidgetItem([name])
            self.tree.addTopLevelItem(item)
            progress_bar = QProgressBar()
            progress_bar.setAlignment(Qt.AlignHCenter)
            self.presenter.add_progress_bar(id, progress_bar)
            cancel_button = CancelButton(self.presenter, id)
            self.tree.setItemWidget(item, 1, progress_bar)
            self.tree.setItemWidget(item, 2, cancel_button)
            for prop in properties:
                item.addChild(QTreeWidgetItem(prop))
