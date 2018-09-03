from PyQt4 import QtGui, QtCore

from six import iteritems

from Muon.GUI.Common.checkbox import Checkbox


class PeakSelectorView(QtGui.QListWidget):
    sig_finished_selection = QtCore.pyqtSignal(object, object)

    def __init__(self, peak_data, element, parent=None):
        super(PeakSelectorView, self).__init__(parent)
        widget = QtGui.QWidget()

        self.new_data = {}
        self.update_new_data(peak_data)
        self.element = element
        self.setWindowTitle(element)
        self.list = QtGui.QVBoxLayout(self)

        primary = peak_data["Primary"]
        self._create_checkbox_list("Primary", primary)
        secondary = peak_data["Secondary"]
        self._create_checkbox_list("Secondary", secondary)

        self.okay_button = QtGui.QPushButton("Okay")
        self.list.addWidget(self.okay_button)
        self.okay_button.clicked.connect(self._finished_selection)

        widget.setLayout(self.list)
        scroll = QtGui.QScrollArea()
        scroll.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        scroll.setWidgetResizable(False)
        scroll.setWidget(widget)

        scroll_layout = QtGui.QVBoxLayout(self)
        scroll_layout.addWidget(scroll)

        self.setLayout(scroll_layout)

    def update_new_data(self, data):
        for el, values in iteritems(data):
            if values is None:
                data[el] = {}
        new_data = data["Primary"].copy()
        new_data.update(data["Secondary"])
        self.new_data = new_data

    def _finished_selection(self):
        self.sig_finished_selection.emit(self.element, self.new_data)

    def _create_checkbox_list(self, heading, checkbox_data):
        _heading = QtGui.QLabel(heading)
        self.list.addWidget(_heading)
        for peak_type, value in iteritems(checkbox_data):
            checkbox = Checkbox("{}: {}".format(peak_type, value))
            checkbox.setChecked(True)
            checkbox.on_checkbox_unchecked(self._remove_value_from_new_data)
            checkbox.on_checkbox_checked(self._add_value_to_new_data)
            self.list.addWidget(checkbox)

    def _parse_checkbox_name(self, name):
        peak_type, value = name.replace(" ", "").split(":")
        return peak_type, value

    def _remove_value_from_new_data(self, checkbox):
        peak_type, _ = self._parse_checkbox_name(checkbox.name)
        del self.new_data[peak_type]

    def _add_value_to_new_data(self, checkbox):
        peak_type, value = self._parse_checkbox_name(checkbox.name)
        self.new_data[peak_type] = value

    def on_okay_pressed(self, slot):
        self.sig_finished_selection.connect(slot)

    def unreg_on_okay_pressed(self, slot):
        self.sig_finished_selection.disconnect(slot)
