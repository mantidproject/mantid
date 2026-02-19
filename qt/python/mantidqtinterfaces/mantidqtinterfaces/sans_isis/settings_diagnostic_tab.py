# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The settings diagnostic tab view.

The settings diagnostic tab allows to display the state information in a tree view. The user can select the data
from the individual rows in the data table. This view is useful for checking the overall settings of a reduction
and helps the developer to identify issues.
"""

from abc import ABCMeta, abstractmethod
import os
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import GENERIC_SETTINGS, JSON_SUFFIX, load_file

unicode = str

Ui_SettingsDiagnosticTab, _ = load_ui(__file__, "settings_diagnostic_tab.ui")


class SettingsDiagnosticTab(QtWidgets.QWidget, Ui_SettingsDiagnosticTab):
    class SettingsDiagnosticTabListener(metaclass=ABCMeta):
        """
        Defines the elements which a presenter can listen to for the diagnostic tab
        """

        @abstractmethod
        def on_row_changed(self):
            pass

        @abstractmethod
        def on_update_rows(self):
            pass

        @abstractmethod
        def on_collapse(self):
            pass

        @abstractmethod
        def on_expand(self):
            pass

        @abstractmethod
        def on_save_state_to_file(self):
            pass

    def __init__(self):
        super(SettingsDiagnosticTab, self).__init__()
        self.setupUi(self)

        # Hook up signal and slots
        self.connect_signals()
        self._settings_diagnostic_listeners = []

        # Excluded settings entries
        self.excluded = ["state_module", "state_name"]

        # Q Settings
        self.__generic_settings = GENERIC_SETTINGS
        self.__save_location_path_key = "save_state_location"

    def add_listener(self, listener):
        if not isinstance(listener, SettingsDiagnosticTab.SettingsDiagnosticTabListener):
            raise ValueError("The listener is not of type SettingsDiagnosticTabListener but rather {}".format(type(listener)))
        self._settings_diagnostic_listeners.append(listener)

    def clear_listeners(self):
        self._settings_diagnostic_listeners = []

    def _call_settings_diagnostic_listeners(self, target):
        for listener in self._settings_diagnostic_listeners:
            target(listener)

    def on_expand(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_expand())

    def on_collapse(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_collapse())

    def on_row_changed(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_row_changed())

    def on_update_rows(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_update_rows())

    def on_save_state(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_save_state_to_file())

    def on_browse_save_location(self):
        load_file(self.save_state_line_edit, "*.json", self.__generic_settings, self.__save_location_path_key, self.get_save_location)

        # Correct the file extension. The output file has to be a json type file. If the user has added a different
        # file extension then change it to .json
        save_location = self.get_save_location()
        path_dir = os.path.dirname(save_location)
        if not path_dir:
            return

        file_name, _ = os.path.splitext(save_location)
        full_file_path = file_name + JSON_SUFFIX
        self.save_state_line_edit.setText(full_file_path)

    def connect_signals(self):
        self.select_row_combo_box.currentIndexChanged.connect(self.on_row_changed)
        self.select_row_push_button.clicked.connect(self.on_update_rows)
        self.collapse_button.clicked.connect(self.on_collapse)
        self.expand_button.clicked.connect(self.on_expand)
        self.save_state_save_push_button.clicked.connect(self.on_save_state)
        self.save_state_browse_push_button.clicked.connect(self.on_browse_save_location)

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def update_rows(self, indices):
        self.select_row_combo_box.blockSignals(True)
        self.select_row_combo_box.clear()
        for index in indices:
            self.select_row_combo_box.addItem(str(index))
        self.select_row_combo_box.blockSignals(False)

    def fill_tree_widget(self, item, value):
        item.setExpanded(True)
        if isinstance(value, dict):
            for key, val in sorted(value.items()):
                if key in self.excluded:
                    continue
                child = QtWidgets.QTreeWidgetItem()
                child.setText(0, unicode(key))
                item.addChild(child)
                self.fill_tree_widget(child, val)
        elif isinstance(value, list):
            for val in value:
                child = QtWidgets.QTreeWidgetItem()
                child.setText(1, unicode(val))
                item.addChild(child)
        else:
            child = QtWidgets.QTreeWidgetItem()
            child.setText(1, unicode(value))
            item.addChild(child)

    def set_row(self, index):
        found_index = self.select_row_combo_box.findText(str(index))
        if found_index and found_index != -1:
            self.select_row_combo_box.setCurrentIndex(found_index)

    def set_tree(self, state_dict):
        self.tree_widget.clear()
        if state_dict:
            self.fill_tree_widget(self.tree_widget.invisibleRootItem(), state_dict)

    def get_current_row(self):
        value = self.select_row_combo_box.currentText()
        if not value:
            value = -1
        return int(value)

    def collapse(self):
        for index in range(self.tree_widget.topLevelItemCount()):
            top_level_item = self.tree_widget.topLevelItem(index)
            self.tree_widget.collapseItem(top_level_item)

    def expand(self):
        for index in range(self.tree_widget.topLevelItemCount()):
            top_level_item = self.tree_widget.topLevelItem(index)
            self.tree_widget.expandItem(top_level_item)

    def get_save_location(self):
        return str(self.save_state_line_edit.text())

    def set_save_location(self, full_file_path):
        self.save_state_line_edit.setText(full_file_path)

    def set_processing(self, processing=True):
        if processing:
            self.select_row_combo_box.setEnabled(False)
            self.expand_button.setEnabled(False)
            self.collapse_button.setEnabled(False)
            self.save_state_line_edit.setEnabled(False)
            self.save_state_browse_push_button.setEnabled(False)
            self.save_state_save_push_button.setEnabled(False)
            self.select_row_push_button.setEnabled(False)
        else:
            self.select_row_combo_box.setEnabled(True)
            self.expand_button.setEnabled(True)
            self.collapse_button.setEnabled(True)
            self.save_state_line_edit.setEnabled(True)
            self.save_state_browse_push_button.setEnabled(True)
            self.save_state_save_push_button.setEnabled(True)
            self.select_row_push_button.setEnabled(True)
