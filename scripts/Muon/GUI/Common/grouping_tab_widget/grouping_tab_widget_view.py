from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui


class GroupingTabView(QtGui.QWidget):

    def __init__(self, grouping_table, pairing_table, parent=None):
        super(GroupingTabView, self).__init__(parent)

        self._grouping_table = grouping_table
        self._pairing_table = pairing_table

        self.setup_interface_layout()

    def setup_description_layout(self):
        self.horizontal_layout_description = QtGui.QHBoxLayout()
        self.horizontal_layout_description.setObjectName("descriptionLayout")

        self.description_label = QtGui.QLabel(self)
        self.description_label.setText("Description : ")
        self.description_label.setToolTip("")
        self.description_label.setObjectName("descriptionLabel")

        self.description_edit = QtGui.QLineEdit(self)
        self.description_edit.setText("")
        self.description_edit.setReadOnly(True)
        self.description_edit.setToolTip("")
        self.description_edit.setObjectName("descriptionEdit")

        self.horizontal_layout_description.addWidget(self.description_label)
        self.horizontal_layout_description.addWidget(self.description_edit)

    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.setup_description_layout()



        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")

        self.load_grouping_button = QtGui.QPushButton(self)
        self.load_grouping_button.setText("Load Grouping")
        self.save_grouping_button = QtGui.QPushButton(self)
        self.save_grouping_button.setText("Save Grouping")
        self.clear_grouping_button = QtGui.QPushButton(self)
        self.clear_grouping_button.setText("Clear Grouping")

        self.horizontal_layout.addWidget(self.load_grouping_button)
        self.horizontal_layout.addWidget(self.save_grouping_button)
        self.horizontal_layout.addWidget(self.clear_grouping_button)


        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")

        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_description)
        if self._grouping_table:
            self.vertical_layout.addWidget(self._grouping_table)
        if self._pairing_table:
            self.vertical_layout.addWidget(self._pairing_table)

        self.horizontal_layout_base = QtGui.QHBoxLayout()
        self.update_button = QtGui.QPushButton(self)
        self.update_button.setText("Update All")
        self.horizontal_layout_base.addWidget(self.update_button)

        self.vertical_layout.addItem(self.horizontal_layout_base)

        self.setLayout(self.vertical_layout)

    def getLayout(self):
        return self.vertical_layout

    def set_grouping_table(self, table):
        self._grouping_table = table

    def set_pairing_table(self, table):
        self._pairing_table = table

    def update_tables(self):
        self._grouping_table.update_view_from_model()
        self._pairing_table.update_view_from_model()

    def set_description_text(self, text):
        self.description_edit.setText(text)

    def on_grouping_table_changed(self, slot):
        self._grouping_table.dataChanged.connect(slot)

    def on_pairing_table_changed(self, slot):
        self._pairing_table.dataChanged.connect(slot)

    def on_add_pair_requested(self, slot):
        self._grouping_table.addPairRequested.connect(slot)

    def on_clear_grouping_button_clicked(self, slot):
        self.clear_grouping_button.clicked.connect(slot)

    def on_update_button_clicked(self,slot):
        self.update_button.clicked.connect(slot)