from qtpy.QtWidgets import QWidget, QPushButton, QComboBox, QTreeWidget, QVBoxLayout, QHBoxLayout
from qtpy.QtCore import QCoreApplication
from .presenter import IAlgorithmSelectorView


class AlgorithmSelectorWidget(QWidget, IAlgorithmSelectorView):
    """
    An algorithm selector view implemented with qtpy.
    """
    def __init__(self, parent=None):
        self.execute_button = None
        self.search_box = None
        self.tree = None
        QWidget.__init__(self, parent)
        IAlgorithmSelectorView.__init__(self)

    def init_ui(self):
        """
        Create and layout the GUI elements.
        """
        self.execute_button = QPushButton('Execute')
        self.search_box = QComboBox(self)
        self.tree = QTreeWidget(self)

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.execute_button)
        top_layout.addWidget(self.search_box)
        top_layout.setStretch(1, 1)

        layout = QVBoxLayout()
        layout.addLayout(top_layout)
        layout.addWidget(self.tree)
        self.setLayout(layout)

    def populate_ui(self, data):
        """
        Populate the GUI elements with the data that comes from Presenter.
        :param data: a list of algorithm descriptors.
        """
        self.search_box.addItems(['A', 'B', 'C'])
        self.search_box.setCurrentIndex(-1)
        print data
