from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui


class GroupingTabView(QtGui.QWidget):

    def __init__(self, grouping_table, pairing_table, parent=None):
        super(GroupingTabView, self).__init__(parent)
        # declare all the interface items in the __init__ method
        self.horizontal_layout = None
        self.load_grouping_button = None
        self.save_grouping_button = None
        self.clear_grouping_button = None
        self.default_grouping_button = None
        self.vertical_layout = None
        self.horizontal_layout_base = None
        self.update_button = None
        self.horizontal_layout_description = None
        self.description_label = None
        self.description_edit = None

        self._grouping_table = grouping_table
        self._pairing_table = pairing_table

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.setup_description_layout()

        self.load_grouping_button = QtGui.QPushButton(self)
        self.load_grouping_button.setText("Load Grouping")
        self.load_grouping_button.setToolTip("Load a previously saved grouping (in XML format)")

        self.save_grouping_button = QtGui.QPushButton(self)
        self.save_grouping_button.setText("Save Grouping")
        self.save_grouping_button.setToolTip("Save the current state of the group/pair table to XML format")

        self.clear_grouping_button = QtGui.QPushButton(self)
        self.clear_grouping_button.setText("Clear Grouping")
        self.clear_grouping_button.setToolTip("Clear the grouping/pairing tables")

        self.default_grouping_button = QtGui.QPushButton(self)
        self.default_grouping_button.setText("Default Grouping")
        self.default_grouping_button.setToolTip("Restore the default grouping for the currently selected instrument.")

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.load_grouping_button)
        self.horizontal_layout.addWidget(self.save_grouping_button)
        self.horizontal_layout.addWidget(self.clear_grouping_button)
        self.horizontal_layout.addWidget(self.default_grouping_button)

        self.horizontal_layout_base = QtGui.QHBoxLayout()
        self.update_button = QtGui.QPushButton(self)
        self.update_button.setText("Update All")
        self.update_button.setToolTip("Calculate group counts and pair asymmetries from the tables and store the data.")
        self.horizontal_layout_base.addWidget(self.update_button)

        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_description)
        if self._grouping_table:
            self.vertical_layout.addWidget(self._grouping_table)
        if self._pairing_table:
            self.vertical_layout.addWidget(self._pairing_table)
        self.vertical_layout.addItem(self.horizontal_layout_base)

        self.setLayout(self.vertical_layout)

    def setup_description_layout(self):
        self.horizontal_layout_description = QtGui.QHBoxLayout()
        self.horizontal_layout_description.setObjectName("descriptionLayout")

        self.description_label = QtGui.QLabel(self)
        self.description_label.setText("Description : ")
        self.description_label.setToolTip("Description of the data : Instrument, number of detectors "
                                          "and main field direction.")
        self.description_label.setObjectName("descriptionLabel")

        self.description_edit = QtGui.QLineEdit(self)
        self.description_edit.setText("")
        self.description_edit.setReadOnly(True)
        self.description_edit.setToolTip("Description of the data : Instrument, number of detectors "
                                         "and main field direction.")
        self.description_edit.setObjectName("descriptionEdit")

        self.horizontal_layout_description.addWidget(self.description_label)
        self.horizontal_layout_description.addWidget(self.description_edit)

    def getLayout(self):
        return self.vertical_layout

    def set_buttons_enabled(self, enabled=True):
        self.load_grouping_button.setEnabled(enabled)
        self.save_grouping_button.setEnabled(enabled)
        self.clear_grouping_button.setEnabled(enabled)
        self.default_grouping_button.setEnabled(enabled)
        self.update_button.setEnabled(enabled)

    def set_grouping_table(self, table):
        self._grouping_table = table

    def set_pairing_table(self, table):
        self._pairing_table = table

    def update_tables(self):
        self._grouping_table.update_view_from_model()
        self._pairing_table.update_view_from_model()

    def set_description_text(self, text):
        self.description_edit.setText(text)

    def show_file_browser_and_return_selection(self, file_filter, search_directories):
        default_directory = search_directories[0]
        chosen_file = QtGui.QFileDialog.getOpenFileName(self, "Select file", default_directory,
                                                        file_filter)
        return str(chosen_file)

    def show_file_save_browser_and_return_selection(self):
        chosen_file = QtGui.QFileDialog.getSaveFileName(self, "Select file")
        return str(chosen_file)

    # ------------------------------------------------------------------------------------------------------------------
    # Signal / slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_grouping_table_changed(self, slot):
        self._grouping_table.dataChanged.connect(slot)

    def on_pairing_table_changed(self, slot):
        self._pairing_table.dataChanged.connect(slot)

    def on_add_pair_requested(self, slot):
        self._grouping_table.addPairRequested.connect(slot)

    def on_clear_grouping_button_clicked(self, slot):
        self.clear_grouping_button.clicked.connect(slot)

    def on_default_grouping_button_clicked(self, slot):
        self.default_grouping_button.clicked.connect(slot)

    def on_update_button_clicked(self, slot):
        self.update_button.clicked.connect(slot)

    def on_load_grouping_button_clicked(self, slot):
        self.load_grouping_button.clicked.connect(slot)

    def on_save_grouping_button_clicked(self, slot):
        self.save_grouping_button.clicked.connect(slot)
