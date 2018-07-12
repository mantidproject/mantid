from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui
from PyQt4.QtCore import pyqtSignal

import ui_summation_settings_widget
from sans.gui_logic.models.binning_type import BinningType


def set_checked_without_signal(checkable, should_be_checked):
    checkable.blockSignals(True)
    checkable.setChecked(should_be_checked)
    checkable.blockSignals(False)


class SummationSettingsWidget(QtGui.QWidget, ui_summation_settings_widget.Ui_SummationSettingsWidget):
    binningTypeChanged = pyqtSignal(int)
    preserveEventsChanged = pyqtSignal(bool)
    binSettingsChanged = pyqtSignal()
    additionalTimeShiftsChanged = pyqtSignal()
    sum = pyqtSignal()

    def __init__(self, parent=None):
        super(SummationSettingsWidget, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def setupUi(self, other):
        ui_summation_settings_widget.Ui_SummationSettingsWidget.setupUi(self, other)
        self._setupBinningTypes()

    def _setupBinningTypes(self):
        binningTypes = [
            'Use custom binning',
            'Use binning from monitors',
            'Save as event data'
        ]
        for binningType in binningTypes:
            self.binningType.addItem(binningType)

    def _connect_signals(self):
        self.binningType.currentIndexChanged.connect(self._handle_binning_type_changed)
        self.overlayEventWorkspacesCheckbox.stateChanged.connect(self._handle_overlay_ews_changed)
        self.binningOptionsLineEdit.editingFinished.connect(self._handle_binning_options_line_edit_changed)

    def _binning_type_index_to_type(self, index):
        if index == 0:
            return BinningType.Custom
        elif index == 1:
            return BinningType.FromMonitors
        elif index == 2:
            return BinningType.SaveAsEventData

    def _handle_binning_type_changed(self, index):
        binning_type = self._binning_type_index_to_type(index)
        self.binningTypeChanged.emit(binning_type)

    def _handle_binning_options_line_edit_changed(self):
        # Since the text box is shared we don't
        # know which of these was actually changed.
        # The presenter can work it out.
        self.binSettingsChanged.emit()
        self.additionalTimeShiftsChanged.emit()

    def _handle_overlay_ews_changed(self, state):
        self.preserveEventsChanged.emit(state != 0)

    def draw_settings(self, settings):
        self._draw_bin_settings(settings)
        self._draw_additional_time_shifts(settings)
        self._draw_overlay_event_workspaces(settings)

    def bin_settings(self):
        return self.binningOptionsLineEdit.text()

    def additional_time_shifts(self):
        return self.binningOptionsLineEdit.text()

    def _draw_overlay_event_workspaces(self, settings):
        if settings.has_overlay_event_workspaces():
            self.overlayEventWorkspacesCheckbox.setVisible(True)
            should_be_checked = settings.is_overlay_event_workspaces_enabled()
            set_checked_without_signal(
                self.overlayEventWorkspacesCheckbox, should_be_checked)
        else:
            set_checked_without_signal(
                self.overlayEventWorkspacesCheckbox, False)
            self.overlayEventWorkspacesCheckbox.setVisible(False)

    def _draw_bin_settings(self, settings):
        if settings.has_bin_settings():
            self._activate_line_edit('Custom Bin Boundaries:', settings.bin_settings)
        elif not settings.has_additional_time_shifts():
            self._deactivate_line_edit()

    def _draw_additional_time_shifts(self, settings):
        if settings.has_additional_time_shifts():
            self._activate_line_edit('Additional Time Shifts:', settings.additional_time_shifts)
        elif not settings.has_bin_settings():
            self._deactivate_line_edit()

    def _activate_line_edit(self, label, content):
        self.binningOptionsLineEdit.setText(content)
        self.binningOptionsLineEdit.setVisible(True)
        self.lineEditLabel.setText(label)
        self.lineEditLabel.setVisible(True)

    def _deactivate_line_edit(self):
        self.binningOptionsLineEdit.setText('')
        self.binningOptionsLineEdit.setVisible(False)
        self.lineEditLabel.setVisible(False)
