# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqtinterfaces.Muon.GUI.Common.message_box import warning, question
from mantidqtinterfaces.Muon.GUI.Common.utilities.muon_file_utils import show_file_browser_and_return_selection

import os


class GroupingTabView(QtWidgets.QWidget):
    def __init__(self, grouping_table, pairing_table, diff_table, parent=None):
        super(GroupingTabView, self).__init__(parent)
        # declare all the interface items in the __init__ method
        self.horizontal_layout = None
        self.load_grouping_button = None
        self.save_grouping_button = None
        self.clear_grouping_button = None
        self.default_grouping_button = None
        self.period_information_button = None
        self.vertical_layout = None
        self.horizontal_layout_base = None
        self.horizontal_layout_description = None
        self.horizontal_layout_1 = None
        self.description_label = None
        self.description_edit = None
        self.summed_period_edit = None
        self.period_label = None
        self.subtracted_period_edit = None
        self.minus_label = None
        self._parent = parent

        self._grouping_table = grouping_table
        self._pairing_table = pairing_table
        self._diff_table = diff_table

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.setup_description_layout()

        self.load_grouping_button = QtWidgets.QPushButton(self)
        self.load_grouping_button.setText("Load")
        self.load_grouping_button.setToolTip("Load a previously saved grouping (in XML format)")

        self.save_grouping_button = QtWidgets.QPushButton(self)
        self.save_grouping_button.setText("Save")
        self.save_grouping_button.setToolTip("Save the current state of the group/pair table to XML format")

        self.clear_grouping_button = QtWidgets.QPushButton(self)
        self.clear_grouping_button.setText("Clear")
        self.clear_grouping_button.setToolTip("Clear the grouping/pairing tables")

        self.default_grouping_button = QtWidgets.QPushButton(self)
        self.default_grouping_button.setText("Default")
        self.default_grouping_button.setToolTip("Restore the default grouping for the currently selected instrument.")

        self.period_information_button = QtWidgets.QPushButton(self)
        self.period_information_button.setText("Periods")
        self.period_information_button.setToolTip("Display a table with further information about the periods of the run(s)")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.load_grouping_button)
        self.horizontal_layout.addWidget(self.save_grouping_button)
        self.horizontal_layout.addWidget(self.clear_grouping_button)
        self.horizontal_layout.addWidget(self.default_grouping_button)
        self.horizontal_layout.addWidget(self.period_information_button)

        self.horizontal_layout_base = QtWidgets.QHBoxLayout()

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_description)
        self.vertical_layout.addWidget(self._grouping_table)
        self.vertical_layout.addWidget(self._pairing_table)
        self.vertical_layout.addWidget(self._diff_table)
        self.vertical_layout.addItem(self.horizontal_layout_base)

        self.setLayout(self.vertical_layout)

    def setup_description_layout(self):
        self.horizontal_layout_description = QtWidgets.QHBoxLayout()
        self.horizontal_layout_description.setObjectName("descriptionLayout")

        self.description_label = QtWidgets.QLabel(self)
        self.description_label.setText("Description : ")
        self.description_label.setToolTip("Description of the data : Instrument, number of detectors and main field direction.")
        self.description_label.setObjectName("descriptionLabel")

        self.description_edit = QtWidgets.QLineEdit(self)
        self.description_edit.setText("")
        self.description_edit.setReadOnly(False)
        self.description_edit.setToolTip("Description of the data : Instrument, number of detectors and main field direction.")
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
        self.period_information_button.setEnabled(enabled)

    def set_grouping_table(self, table):
        self._grouping_table = table

    def set_pairing_table(self, table):
        self._pairing_table = table

    def update_tables(self):
        self._grouping_table.update_view_from_model()
        self._pairing_table.update_view_from_model()
        self._diff_table.update_view_from_model()

    def set_description_text(self, text):
        self.description_edit.setText(text)

    def get_description_text(self):
        return self.description_edit.text()

    def show_file_browser_and_return_selection(self, file_filter, search_directories):
        return show_file_browser_and_return_selection(self, file_filter, search_directories)[0]

    def show_file_save_browser_and_return_selection(self):
        chosen_file, _filter = QtWidgets.QFileDialog.getSaveFileName(self, "Select file", "", "XML files (*.xml)")
        chosen_file = str(chosen_file)
        if chosen_file == "":
            return chosen_file

        path_extension = os.path.splitext(chosen_file)

        if path_extension[1] == ".xml":
            return chosen_file
        else:
            updated_file = path_extension[0] + ".xml"
            if os.path.isfile(updated_file):
                if question("File {} already exists do you want to overwrite it?".format(updated_file), parent=self):
                    return path_extension[0] + ".xml"
                else:
                    return ""
            return path_extension[0] + ".xml"

    def display_warning_box(self, message):
        warning(message, self)

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

    def on_load_grouping_button_clicked(self, slot):
        self.load_grouping_button.clicked.connect(slot)

    def on_save_grouping_button_clicked(self, slot):
        self.save_grouping_button.clicked.connect(slot)

    def on_period_information_button_clicked(self, slot):
        self.period_information_button.clicked.connect(slot)
