# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqtinterfaces.Muon.GUI.Common.message_box import warning
from mantidqtinterfaces.Muon.GUI.Common.utilities.muon_file_utils import show_file_browser_and_return_selection


class EAGroupingTabView(QtWidgets.QWidget):
    def __init__(self, grouping_table, parent=None):
        super(EAGroupingTabView, self).__init__(parent)
        # declare all the interface items in the __init__ method
        self.horizontal_layout = None
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

        self._grouping_table = grouping_table

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.setObjectName("GroupingTabView")
        self.resize(1000, 1000)

        self.setup_description_layout()

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")

        self.horizontal_layout_base = QtWidgets.QHBoxLayout()

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_description)
        self.vertical_layout.addWidget(self._grouping_table)
        self.vertical_layout.addItem(self.horizontal_layout_base)

        self.setLayout(self.vertical_layout)

    def setup_description_layout(self):
        self.horizontal_layout_description = QtWidgets.QHBoxLayout()
        self.horizontal_layout_description.setObjectName("descriptionLayout")

        self.description_label = QtWidgets.QLabel(self)
        self.description_label.setText("Description : ")
        self.description_label.setToolTip("Description of the data : \u03bdx and file type")
        self.description_label.setObjectName("descriptionLabel")

        self.description_edit = QtWidgets.QLineEdit(self)
        self.description_edit.setText("")
        self.description_edit.setReadOnly(False)
        self.description_edit.setToolTip("Description of the data : \u03bdx and file type. ")
        self.description_edit.setObjectName("descriptionEdit")

        self.horizontal_layout_description.addWidget(self.description_label)
        self.horizontal_layout_description.addWidget(self.description_edit)

    def getLayout(self):
        return self.vertical_layout

    def set_grouping_table(self, table):
        self._grouping_table = table

    def update_tables(self):
        self._grouping_table.update_view_from_model()

    def set_description_text(self, text):
        self.description_edit.setText(text)

    def get_description_text(self):
        return self.description_edit.text()

    def show_file_browser_and_return_selection(self, file_filter, search_directories):
        return show_file_browser_and_return_selection(self, file_filter, search_directories)[0]

    def display_warning_box(self, message):
        warning(message, self)

    # ------------------------------------------------------------------------------------------------------------------
    # Signal / slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_grouping_table_changed(self, slot):
        self._grouping_table.dataChanged.connect(slot)
