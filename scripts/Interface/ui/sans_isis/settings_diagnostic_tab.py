import ui_settings_diagnostic_tab
from PyQt4 import QtGui, QtCore
from abc import ABCMeta, abstractmethod
from six import with_metaclass


class SettingsDiagnosticTab(QtGui.QWidget, ui_settings_diagnostic_tab.Ui_SettingsDiagnosticTab):
    class SettingsDiagnosticTabListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to for the diagnostic tab
        """
        @abstractmethod
        def on_row_changed(self):
            pass

        @abstractmethod
        def on_update_rows(self):
            pass

    def __init__(self):
        super(SettingsDiagnosticTab, self).__init__()
        self.setupUi(self)

        # Hook up signal and slots
        self.connect_signals()
        self._settings_diagnostic_listeners = []

        # Excluded settings entries
        self.excluded = ["state_module", "state_name"]
        self.class_type_id = "ClassTypeParameter"

    def add_listener(self, listener):
        if not isinstance(listener, SettingsDiagnosticTab.SettingsDiagnosticTabListener):
            raise ValueError("The listener ist not of type SettingsDiagnosticTabListener but rather {}".format(type(listener)))
        self._settings_diagnostic_listeners.append(listener)

    def clear_listeners(self):
        self._settings_diagnostic_listeners = []

    def _call_settings_diagnostic_listeners(self, target):
        for listener in self._settings_diagnostic_listeners:
            target(listener)

    def on_row_changed(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_row_changed())

    def on_update_rows(self):
        self._call_settings_diagnostic_listeners(lambda listener: listener.on_update_rows())

    def connect_signals(self):
        self.select_row_combo_box.currentIndexChanged.connect(self.on_row_changed)
        self.select_row_push_button.clicked.connect(self.on_update_rows)

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
        if type(value) is dict:
            for key, val in sorted(value.iteritems()):
                if key in self.excluded:
                    continue
                child = QtGui.QTreeWidgetItem()
                child.setText(0, unicode(key))
                item.addChild(child)
                self.fill_tree_widget(child, val)
        elif type(value) is list:
            for val in value:
                child = QtGui.QTreeWidgetItem()
                child.setText(1, unicode(val))
                item.addChild(child)
        else:
            child = QtGui.QTreeWidgetItem()
            value = self.clean_class_type(value)
            child.setText(1, unicode(value))
            item.addChild(child)

    def clean_class_type(self, value):
        if isinstance(value, str) and self.class_type_id in value:
            # Only the last element is of interest
            split_values = value.split("#")
            return split_values[-1]
        else:
            return value

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
