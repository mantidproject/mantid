# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from .WorkspaceItem import WorkspaceItem
from .SpectrumItem import SpectrumItem

from qtpy.QtWidgets import QDockWidget, QWidget, QHeaderView
from qtpy.QtGui import QColor
from qtpy.QtCore import *
from qtpy import uic

import os


class SuperplotViewSide(QDockWidget):

    UI = "superplotSide.ui"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewSide.UI), self)
        ws_list = self.workspacesList
        ws_list.header().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        ws_list.header().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        size0 = ws_list.header().sectionSize(0)
        size1 = ws_list.header().sectionSize(1)
        ws_list.header().setDefaultSectionSize(size1)
        ws_list.header().setSectionResizeMode(QHeaderView.Stretch)
        ws_list.header().setSectionResizeMode(1, QHeaderView.Interactive)
        ws_list.setMinimumSize(QSize(size0 + size1, 0))
        self.workspaceSelector.setWorkspaceTypes(["Workspace2D",
                                                  "WorkspaceGroup",
                                                  "EventWorkspace"])


class SuperplotViewBottom(QDockWidget):

    UI = "superplotBottom.ui"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewBottom.UI), self)
        self.setFocusPolicy(Qt.ClickFocus)

    def keyPressEvent(self, event):
        """
        Override of QDockWidget::keyPressEvent. This method forward some key
        press event to some relevant actions of the contained widgets.

        Args:
            event (QKeyEvent): the key press event
        """
        if event.key() == Qt.Key_Left:
            self.spectrumSlider.keyPressEvent(event)
        elif event.key() == Qt.Key_Right:
            self.spectrumSlider.keyPressEvent(event)
        elif event.key() == Qt.Key_Space:
            self.holdButton.click()


class SuperplotView(QWidget):

    _presenter = None
    _side_view = None
    _bottom_view = None

    def __init__(self, presenter, parent=None):
        super().__init__(parent)
        self._presenter = presenter
        self._side_view = SuperplotViewSide(self)
        self._bottom_view = SuperplotViewBottom(self)

        side = self._side_view
        side.visibilityChanged.connect(self._presenter.onVisibilityChanged)
        side.addButton.clicked.connect(self._presenter.onAddButtonClicked)
        side.workspacesList.itemSelectionChanged.connect(
                self._presenter.onWorkspaceSelectionChanged)
        bottom = self._bottom_view
        bottom.holdButton.toggled.connect(self._presenter.onHoldButtonToggled)
        bottom.spectrumSlider.valueChanged.connect(
                self._presenter.onSpectrumSliderMoved)
        bottom.spectrumSpinBox.valueChanged.connect(
                self._presenter.onSpectrumSpinBoxChanged)
        bottom.modeComboBox.currentTextChanged.connect(
                self._presenter.onModeChanged)

    def get_side_widget(self):
        return self._side_view

    def get_bottom_widget(self):
        return self._bottom_view

    def close(self):
        self._side_view.close()
        self._bottom_view.close()

    def get_selected_workspace(self):
        """
        Get the workspace selected in the workspace selector.

        Return:
            str: name of the workspace
        """
        return self._side_view.workspaceSelector.currentText()

    def set_selection(self, selection):
        """
        Set the selected workspaces and spectra in list.

        Args:
            selection (dict(str: list(int))): workspace names and list of
                                              spectrum numbers
        """
        self._side_view.workspacesList.blockSignals(True)
        self._side_view.workspacesList.clearSelection()
        for i in range(self._side_view.workspacesList.topLevelItemCount()):
            ws_item = self._side_view.workspacesList.topLevelItem(i)
            ws_name = ws_item.get_workspace_name()
            if ws_name in selection:
                if -1 in selection[ws_name]:
                    ws_item.setSelected(True)
                for j in range(ws_item.childCount()):
                    sp_item = ws_item.child(j)
                    if sp_item.get_spectrum_index() in selection[ws_name]:
                        sp_item.setSelected(True)
        self._side_view.workspacesList.blockSignals(False)

    def get_selection(self):
        """
        Get the current selection.

        Returns:
            dict(str: list(int)): workspace names and list of spectrum numbers
        """
        selection = dict()
        items = self._side_view.workspacesList.selectedItems()
        for item in items:
            if item.parent() is None:
                ws_name = item.get_workspace_name()
                if ws_name not in selection:
                    selection[ws_name] = [-1]
            else:
                spectrum = item.get_spectrum_index()
                ws_name = item.parent().get_workspace_name()
                if ws_name in selection:
                    selection[ws_name].append(spectrum)
                else:
                    selection[ws_name] = [spectrum]
        return selection

    def modify_spectrum_label(self, ws_name, spectrum_index, label, color):
        """
        Modify spectrum label text and color.

        Args:
            ws_name (str): name of the concerned workspace
            spectrum_index (int): index of the spectrum
            label (str): new label
            color (str): new color (#RRGGBB)
        """
        ws_item = self._side_view.workspacesList.findItems(ws_name,
                                                           Qt.MatchExactly, 0)
        if not ws_item:
            return
        ws_item = ws_item[0]

        for i in range(ws_item.childCount()):
            sp_item = ws_item.child(i)
            if sp_item.get_spectrum_index() == spectrum_index:
                brush = sp_item.foreground(0)
                brush.setColor(QColor(color))
                sp_item.setForeground(0, brush)
                sp_item.setText(0, label)

    def set_workspaces_list(self, names):
        """
        Set the list of selected workspaces and update the workspace slider
        length.

        Args:
            names (list(str)): list if the workspace names
        """
        self._side_view.workspacesList.blockSignals(True)
        self._side_view.workspacesList.clear()
        for name in names:
            item = WorkspaceItem(self._side_view.workspacesList, name)
            item.signals.sig_del_clicked.connect(
                    self._presenter.onDelButtonClicked)
            item.setText(0, name)
        self._side_view.workspacesList.blockSignals(False)

    def set_spectra_list(self, name, nums):
        """
        Set the list of spectrum index for a workspace in the list.

        Args:
            name (str): name of the workspace
            nums (list(int)): list of the spectrum indexes
        """
        self._side_view.workspacesList.blockSignals(True)
        ws_item = self._side_view.workspacesList.findItems(
                name, Qt.MatchExactly, 0)[0]
        ws_item.takeChildren()
        for num in nums:
            item = SpectrumItem(ws_item, num)
            item.signals.sig_del_clicked.connect(
                    self._presenter.onDelSpectrumButtonClicked)
            item.setText(0, str(num))
        self._side_view.workspacesList.expandAll()
        self._side_view.workspacesList.blockSignals(False)

    def get_spectra_list(self, name):
        """
        Get the list of spectrum indexes for a workspace in the list.

        Args:
            name (str): name of the workspace

        Returns:
            list(int): list of the spectrum indexes
        """
        ws_item = self._side_view.workspacesList.findItems(name,
                                                           Qt.MatchExactly,
                                                           0)
        if ws_item:
            ws_item = ws_item[0]
        else:
            return []
        nums = list()
        for i in range(ws_item.childCount()):
            nums.append(ws_item.child(i).get_spectrum_index())
        return nums

    def check_hold_button(self, state):
        """
        Set the check state of the hold button. This function does not trigger
        the button signals.

        Args:
            state (bool): check state
        """
        self._bottom_view.holdButton.blockSignals(True)
        self._bottom_view.holdButton.setChecked(state)
        self._bottom_view.holdButton.blockSignals(False)

    def set_spectrum_disabled(self, state):
        """
        Disable/enable the spectrum selection widgets (slider and spinbox).

        Args:
            state (bool): if True, disable the widgets
        """
        self._bottom_view.spectrumSlider.setDisabled(state)
        self._bottom_view.spectrumSpinBox.setDisabled(state)

    def is_spectrum_disabled(self):
        """
        Get the state of the spectrum selection widgets.

        Returns:
            True if disabled
        """
        return not self._bottom_view.spectrumSlider.isEnabled()

    def set_spectrum_slider_max(self, length):
        """
        Set the max value of the spectrum slider.

        Args:
            value (int): slider maximum value
        """
        self._bottom_view.spectrumSlider.setMaximum(length)

    def set_spectrum_slider_position(self, position):
        """
        Set the spectrum slider position. This function does not trigger any
        QSlider signals.

        Args:
            position (int): position
        """
        self._bottom_view.spectrumSlider.blockSignals(True)
        self._bottom_view.spectrumSlider.setSliderPosition(position)
        self._bottom_view.spectrumSlider.blockSignals(False)

    def get_spectrum_slider_position(self):
        """
        Get the spectrum slider position.

        Returns:
            int: slider position
        """
        return self._bottom_view.spectrumSlider.value()

    def set_spectrum_spin_box_max(self, value):
        """
        Set the max value of the spectrum spinbox.

        Args:
            value (int): spinbox max value
        """
        self._bottom_view.spectrumSpinBox.setMaximum(value)

    def set_spectrum_spin_box_value(self, value):
        """
        Set the spectrum spinbox value. This function does not trigger any
        spinbox signals.

        Args:
            value (int): spinbox value
        """
        self._bottom_view.spectrumSpinBox.blockSignals(True)
        self._bottom_view.spectrumSpinBox.setValue(value)
        self._bottom_view.spectrumSpinBox.blockSignals(False)

    def get_spectrum_spin_box_value(self):
        """
        Get the current spectrum spinbox value.

        Returns:
            int: spinbox value
        """
        return self._bottom_view.spectrumSpinBox.value()

    def set_available_modes(self, modes):
        """
        Set available modes in the mode combobox.

        Args:
            modes (list(str)): list of modes
        """
        self._bottom_view.modeComboBox.blockSignals(True)
        self._bottom_view.modeComboBox.clear()
        self._bottom_view.modeComboBox.addItems(modes)
        self._bottom_view.modeComboBox.blockSignals(False)

    def set_mode(self, mode):
        """
        Set the selected mode. This method does not trigegr any combobox signal.

        Args:
            mode (str): mode
        """
        self._bottom_view.modeComboBox.blockSignals(True)
        self._bottom_view.modeComboBox.setCurrentText(mode)
        self._bottom_view.modeComboBox.blockSignals(False)

    def get_mode(self):
        """
        Get the selected mode from the combobox.

        Returns:
            str: mode
        """
        return self._bottom_view.modeComboBox.currentText()
