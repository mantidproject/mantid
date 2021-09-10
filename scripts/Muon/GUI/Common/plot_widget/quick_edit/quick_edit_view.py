# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.plot_widget.quick_edit.axis_changer.axis_changer_presenter import AxisChangerWidget


class QuickEditView(QtWidgets.QWidget):
    error_signal = QtCore.Signal(object)

    def __init__(self, subcontext, parent=None, default_msg = "All"):
        super(QuickEditView, self).__init__(parent)
        self._default_selector_msg = default_msg
        button_layout = QtWidgets.QHBoxLayout()
        self.plot_selector = QtWidgets.QComboBox()
        self.plot_selector.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        self.plot_selector.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.plot_selector.setMinimumContentsLength(12)
        self.plot_selector.setEditable(True)
        self.plot_selector.completer().setCompletionMode(
            QtWidgets.QCompleter.PopupCompletion)
        self.plot_selector.view().setMinimumWidth(100)

        self.plot_selector.completer().setFilterMode(
            QtCore.Qt.MatchContains)

        self.plot_selector.addItem(self._default_selector_msg)
        self.plot_selector.setEditable(False)
        self.x_axis_changer = AxisChangerWidget("X", self)

        self.autoscale = None
        self.autoscale = QtWidgets.QCheckBox("Autoscale y")
        self.autoscale.setToolTip("While pan or zoom are enabled autoscale is disabled")

        self.y_axis_changer = AxisChangerWidget("Y", self)

        self.errors = QtWidgets.QCheckBox("Errors")
        self.errors.stateChanged.connect(self._emit_errors)

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.autoscale)
        button_layout.addWidget(self.y_axis_changer.view)
        button_layout.addWidget(self.errors)
        self.setLayout(button_layout)

    @property
    def get_multiple_selection_name(self):
        return self._default_selector_msg

    """ plot selection """
    def disable_plot_selection(self):
        self.plot_selector.setEnabled(False)

    def add_subplot(self, name):
        self.plot_selector.blockSignals(True)
        self.plot_selector.addItem(name)
        self.plot_selector.adjustSize()
        self.plot_selector.blockSignals(False)

    def rm_subplot(self, index):
        self.plot_selector.removeItem(index)
        self.plot_selector.adjustSize()

    def current_selection(self):
        return self.plot_selector.currentText()

    def find_subplot(self, name):
        return self.plot_selector.findText(name)

    def set_selection(self, index: int):
        self.plot_selector.setCurrentIndex(index)

    def get_selection_index(self) -> int:
        return self.plot_selector.currentIndex()

    def plot_at_index(self, index):
        return self.plot_selector.itemText(index)

    def number_of_plots(self):
        return self.plot_selector.count()

    def clear_subplots(self):
        self.plot_selector.blockSignals(True)
        self.plot_selector.clear()
        self.plot_selector.addItem(self._default_selector_msg)
        self.plot_selector.blockSignals(False)

    def connect_plot_selection(self, slot):
        self.plot_selector.currentIndexChanged.connect(slot)

    """ x axis selection """

    def connect_x_range_changed(self, slot):
        self.x_axis_changer.on_range_changed(slot)

    def set_plot_x_range(self, limits):
        self.x_axis_changer.set_limits(limits)

    def get_x_bounds(self):
        return self.x_axis_changer.get_limits()

    """ y axis selection """
    def connect_y_range_changed(self, slot):
        self.y_axis_changer.on_range_changed(slot)

    def set_plot_y_range(self, limits):
        self.y_axis_changer.set_limits(limits)

    def get_y_bounds(self):
        return self.y_axis_changer.get_limits()

    def disable_yaxis_changer(self):
        self.y_axis_changer.view.setEnabled(False)

    def enable_yaxis_changer(self):
        self.y_axis_changer.view.setEnabled(True)

    """ auto scale selection """

    def connect_autoscale_changed(self, slot):
        self.autoscale.clicked.connect(slot)

    @property
    def autoscale_state(self):
        return self.autoscale.checkState()

    def disable_autoscale(self):
        self.autoscale.setEnabled(False)

    def enable_autoscale(self):
        self.autoscale.setEnabled(True)

    def set_autoscale(self, state:bool):
        self.autoscale.setChecked(state)

    def uncheck_autoscale(self):
        self.autoscale.setChecked(False)

    """ errors selection """

    # need our own signal that sends a bool

    def _emit_errors(self):
        state = self.get_errors()
        self.error_signal.emit(state)

    def connect_errors_changed(self, slot):
        self.error_signal.connect(slot)

    def set_errors(self, state):
        self.errors.setChecked(state)

    def get_errors(self):
        return self.errors.isChecked()
