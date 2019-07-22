# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)
from qtpy import QtWidgets, QtCore

from six import iteritems

from Muon.GUI.Common.checkbox import Checkbox


# Check that the new data format contains at least A, Z, primary (they can be empty)
def valid_data(peak_data):
    data_label = peak_data.keys()
    if any(['Z' not in data_label,
            'A' not in data_label,
            'Primary' not in data_label,
            'Secondary' not in data_label]):
        return False

    return True


class PeakSelectorView(QtWidgets.QListWidget):
    sig_finished_selection = QtCore.Signal(object, object)

    def __init__(self, peak_data, element, parent=None):
        super(PeakSelectorView, self).__init__(parent)
        widget = QtWidgets.QWidget()

        self.new_data = {}
        if not valid_data(peak_data):
            raise ValueError

        self.element = element
        self.update_new_data(peak_data)
        self.setWindowTitle(element)
        self.list = QtWidgets.QVBoxLayout(self)

        # Gamma peaks might not be present, if so return empty list
        primary = peak_data["Primary"]
        self.primary_checkboxes = self._create_checkbox_list("Primary", primary)
        secondary = peak_data["Secondary"]
        self.secondary_checkboxes = self._create_checkbox_list("Secondary", secondary, checked=False)
        try:
            gammas = peak_data["Gammas"]
            self.gamma_checkboxes = self._create_checkbox_list("Gammas", gammas, checked=False)
        except KeyError:
            self.gamma_checkboxes = []
        try:
            electrons = peak_data["Electrons"]
            self.electron_checkboxes = self._create_checkbox_list("Electrons", electrons, checked=False)
        except KeyError:
            self.electron_checkboxes = []

        widget.setLayout(self.list)
        scroll = QtWidgets.QScrollArea()
        scroll.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        scroll.setWidgetResizable(False)
        scroll.setWidget(widget)

        scroll_layout = QtWidgets.QVBoxLayout(self)
        scroll_layout.addWidget(scroll)

        self.setLayout(scroll_layout)

    def finish_selection(self):
        self.sig_finished_selection.emit(self.element, self.new_data)

    def closeEvent(self, event):
        self.finish_selection()
        event.accept()

    def get_checked(self):
        return self.new_data

    def update_new_data(self, data):
        for el, values in iteritems(data):
            if values is None:
                data[el] = {}
        new_data = data["Primary"].copy()

        self.new_data = new_data

    def _setup_checkbox(self, name, checked):
        checkbox = Checkbox(name)
        checkbox.setChecked(checked)
        checkbox.on_checkbox_unchecked(self._remove_value_from_new_data)
        checkbox.on_checkbox_checked(self._add_value_to_new_data)
        self.list.addWidget(checkbox)
        return checkbox

    def _create_checkbox_list(self, heading, checkbox_data, checked=True):
        _heading = QtWidgets.QLabel(heading)
        self.list.addWidget(_heading)
        checkboxes = []
        for peak_type, value in iteritems(checkbox_data):
            checkboxes.append(
                self._setup_checkbox(
                    "{}: {}".format(peak_type, value), checked))
        return checkboxes

    def _parse_checkbox_name(self, name):
        peak_type, value = name.replace(" ", "").split(":")
        return peak_type, value

    def _remove_value_from_new_data(self, checkbox):
        peak_type, _ = self._parse_checkbox_name(checkbox.name)
        del self.new_data[peak_type]

    def _add_value_to_new_data(self, checkbox):
        peak_type, value = self._parse_checkbox_name(checkbox.name)
        self.new_data[peak_type] = value

    def on_finished(self, slot):
        self.sig_finished_selection.connect(slot)

    def unreg_on_finished(self, slot):
        self.sig_finished_selection.disconnect(slot)
