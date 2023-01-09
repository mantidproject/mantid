# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Signal

from mantidqt.utils.qt import load_ui
from sans.common.enums import BinningType

Ui_SummationSettingsWidget, _ = load_ui(__file__, "summation_settings_widget.ui")


def set_checked_without_signal(checkable, should_be_checked):
    checkable.blockSignals(True)
    checkable.setChecked(should_be_checked)
    checkable.blockSignals(False)


class SummationSettingsWidget(QtWidgets.QWidget, Ui_SummationSettingsWidget):
    binningTypeChanged = Signal(int)
    preserveEventsChanged = Signal(bool)
    binSettingsChanged = Signal()
    additionalTimeShiftsChanged = Signal()
    sum = Signal()

    def __init__(self, parent=None):
        super(SummationSettingsWidget, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def setupUi(self, other):
        Ui_SummationSettingsWidget.setupUi(self, other)
        self._setupBinningTypes()

    def _setupBinningTypes(self):
        binningTypes = ["Use custom binning", "Use binning from monitors", "Save as event data"]
        for binningType in binningTypes:
            self.binningType.addItem(binningType)

    def _connect_signals(self):
        self.binningType.currentIndexChanged.connect(self._handle_binning_type_changed)
        self.overlayEventWorkspacesCheckbox.stateChanged.connect(self._handle_overlay_ews_changed)
        self.binningOptionsLineEdit.editingFinished.connect(self._handle_binning_options_line_edit_changed)

    @staticmethod
    def _binning_type_to_index(bin_type):
        if bin_type == BinningType.CUSTOM:
            return 0
        elif bin_type == BinningType.FROM_MONITORS:
            return 1
        elif bin_type == BinningType.SAVE_AS_EVENT_DATA:
            return 2

    def _handle_binning_type_changed(self, index):
        self.binningTypeChanged.emit(index)

    def _handle_binning_options_line_edit_changed(self):
        # Since the text box is shared we don't
        # know which of these was actually changed.
        # The presenter can work it out.
        self.binSettingsChanged.emit()
        self.additionalTimeShiftsChanged.emit()

    def _handle_overlay_ews_changed(self, state):
        self.preserveEventsChanged.emit(state != 0)

    def draw_settings(self, settings):
        self._draw_binning_type(settings)
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
            set_checked_without_signal(self.overlayEventWorkspacesCheckbox, should_be_checked)
        else:
            set_checked_without_signal(self.overlayEventWorkspacesCheckbox, False)
            self.overlayEventWorkspacesCheckbox.setVisible(False)

    def _draw_binning_type(self, settings):
        index = self._binning_type_to_index(settings.type)
        if index is not None:
            self.binningType.setCurrentIndex(index)

    def _draw_bin_settings(self, settings):
        if settings.has_bin_settings():
            self._activate_line_edit("Custom Bin Boundaries:", settings.bin_settings)
        elif not settings.has_additional_time_shifts():
            self._deactivate_line_edit()

    def _draw_additional_time_shifts(self, settings):
        if settings.has_additional_time_shifts():
            self._activate_line_edit("Additional Time Shifts:", settings.additional_time_shifts)
        elif not settings.has_bin_settings():
            self._deactivate_line_edit()

    def _activate_line_edit(self, label, content):
        self.binningOptionsLineEdit.setText(content)
        self.binningOptionsLineEdit.setVisible(True)
        self.lineEditLabel.setText(label)
        self.lineEditLabel.setVisible(True)

    def _deactivate_line_edit(self):
        self.binningOptionsLineEdit.setText("")
        self.binningOptionsLineEdit.setVisible(False)
        self.lineEditLabel.setVisible(False)
