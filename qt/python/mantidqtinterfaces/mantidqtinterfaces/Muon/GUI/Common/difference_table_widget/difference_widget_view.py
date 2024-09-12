# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqtinterfaces.Muon.GUI.Common import message_box


class DifferenceView(QtWidgets.QWidget):
    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, pair_table, group_table, parent=None):
        super(DifferenceView, self).__init__(parent)
        # declare all the interface items in the __init__ method
        self.horizontal_layout = None
        self.pair_button = None
        self.group_button = None

        self.vertical_layout = None
        self._group_table = group_table
        self._pair_table = pair_table

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.label = QtWidgets.QLabel("Difference Table for:")
        self.pair_button = QtWidgets.QRadioButton("Pairs")
        self.pair_button.setChecked(True)
        self.group_button = QtWidgets.QRadioButton("Groups")
        self.pair_button.toggled.connect(self.change)
        self.group_button.toggled.connect(self.change)

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.label)
        self.horizontal_layout.addWidget(self.pair_button)
        self.horizontal_layout.addWidget(self.group_button)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addLayout(self.horizontal_layout)
        self.vertical_layout.addWidget(self._group_table)
        self.vertical_layout.addWidget(self._pair_table)
        self.vertical_layout.setContentsMargins(0, 0, 0, 0)
        self._group_table.hide()
        self.setLayout(self.vertical_layout)

    def change(self):
        if self.group_button.isChecked():
            self._pair_table.hide()
            self._group_table.show()
        elif self.pair_button.isChecked():
            self._group_table.hide()
            self._pair_table.show()
