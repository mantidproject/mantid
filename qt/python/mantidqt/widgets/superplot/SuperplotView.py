# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from .WorkspaceItem import WorkspaceItem
from .SpectrumItem import SpectrumItem

from qtpy.QtWidgets import QDockWidget, QWidget
from qtpy.QtGui import QColor
from qtpy.QtCore import *
from qtpy import uic

import os


class SuperplotViewSide(QDockWidget):

    UI = "superplotSide.ui"

    """
    Emitted when the widget is resized.
    """
    resized = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewSide.UI), self)

    def resizeEvent(self, event):
        """
        Override of QWidget::resizeEvent.
        """
        super().resizeEvent(event)
        self.resized.emit()


class SuperplotViewBottom(QDockWidget):

    UI = "superplotBottom.ui"

    """
    Emitted when the widget is resized.
    """
    resized = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewBottom.UI), self)

    def resizeEvent(self, event):
        """
        Override of QWidget::resizeEvent.
        """
        super().resizeEvent(event)
        self.resized.emit()


class SuperplotView(QWidget):

    _presenter = None
    _sideView = None
    _bottomView = None

    def __init__(self, presenter, parent=None):
        super().__init__(parent)
        self._presenter = presenter
        self._sideView = SuperplotViewSide(self)
        self._bottomView = SuperplotViewBottom(self)

        side = self._sideView
        side.resized.connect(self._presenter.onResize)
        side.visibilityChanged.connect(self._presenter.onVisibilityChanged)
        side.addButton.clicked.connect(self._presenter.onAddButtonClicked)
        side.workspacesList.itemSelectionChanged.connect(
                self._presenter.onWorkspaceSelectionChanged)
        bottom = self._bottomView
        bottom.resized.connect(self._presenter.onResize)
        bottom.holdButton.toggled.connect(self._presenter.onHoldButtonToggled)
        bottom.spectrumSlider.valueChanged.connect(
                self._presenter.onSpectrumSliderMoved)
        bottom.spectrumSpinBox.valueChanged.connect(
                self._presenter.onSpectrumSpinBoxChanged)
        bottom.modeComboBox.currentTextChanged.connect(
                self._presenter.onModeChanged)

    def getSideWidget(self):
        return self._sideView

    def getBottomWidget(self):
        return self._bottomView

    def close(self):
        self._sideView.close()
        self._bottomView.close()

    def getSelectedWorkspace(self):
        """
        Get the workspace selected in the workspace selector.

        Return:
            str: name of the workspace
        """
        return self._sideView.workspaceSelector.currentText()

    def getSelectedWorkspaceFromList(self):
        """
        Get the selected workspace from the selection list.

        Returns:
            str: name of the selected workspace, None if nothing is selected
        """
        item = self._sideView.workspacesList.currentItem()
        if item:
            if item.parent() is not None:
                item = item.parent()
            return item.getWorkspaceName()
        else:
            return None

    def setSelectedWorkspacesInList(self, names):
        """
        Select the corresponding worspaces from the list. This function does not
        raise any QTreeWidget signals.

        Args:
            names (list(str)): list of workspace names
        """
        self._sideView.workspacesList.blockSignals(True)
        self._sideView.workspacesList.clearSelection()
        for name in names:
            item = self._sideView.workspacesList.findItems(name,
                                                           Qt.MatchExactly, 0)
            if item:
                self._sideView.workspacesList.setCurrentItem(
                        item[0], 0, QItemSelectionModel.Select)
        self._sideView.workspacesList.expandAll()
        self._sideView.workspacesList.blockSignals(False)

    def getSelectedWorkspacesFromList(self):
        """
        Get the selected workspaces from the selection tree.

        Returns:
            list(str): list of selected workspace names
        """
        items = self._sideView.workspacesList.selectedItems()
        selection = list()
        for item in items:
            if item.parent() is not None:
                item = item.parent()
            wsName = item.getWorkspaceName()
            if wsName not in selection:
                selection.append(wsName)
        return selection

    def setSelection(self, selection):
        """
        Set the selected workspaces and spectra in list.

        Args:
            selection (dict(str: list(int))): workspace names and list of
                                              spectrum numbers
        """
        self._sideView.workspacesList.blockSignals(True)
        self._sideView.workspacesList.clearSelection()
        for i in range(self._sideView.workspacesList.topLevelItemCount()):
            wsItem = self._sideView.workspacesList.topLevelItem(i)
            wsName = wsItem.getWorkspaceName()
            if wsName in selection:
                if not selection[wsName]:
                    wsItem.setSelected(True)
                    continue
                for j in range(wsItem.childCount()):
                    spItem = wsItem.child(j)
                    if spItem.getSpectrumIndex() in selection[wsName]:
                        spItem.setSelected(True)
        self._sideView.workspacesList.blockSignals(False)

    def getSelection(self):
        """
        Get the current selection.

        Returns:
            dict(str: list(int)): workspace names and list of spectrum numbers
        """
        selection = dict()
        items = self._sideView.workspacesList.selectedItems()
        for item in items:
            if item.parent() is None:
                wsName = item.getWorkspaceName()
                if wsName not in selection:
                    selection[wsName] = list()
            else:
                spectrum = item.getSpectrumIndex()
                wsName = item.parent().getWorkspaceName()
                if wsName in selection:
                    selection[wsName].append(spectrum)
                else:
                    selection[wsName] = [spectrum]
        return selection

    def appendWorkspace(self, name):
        """
        Add a workspace at the end of the list.

        Args:
            name (str): name of the workspace
        """
        item = WorkspaceItem(self._sideView.workspacesList, name)
        item.signals.delClicked.connect(self._presenter.onDelButtonClicked)
        item.setText(0, name)

    def removeWorkspace(self, name):
        """
        Remove a workspace from the list.

        Args:
            name (str): name of the workspace
        """
        self._sideView.workspacesList.blockSignals(True)
        wsItem = self._sideView.workspacesList.findItems(name,
                                                         Qt.MatchExactly, 0)
        for item in wsItem:
            index = self._sideView.workspacesList.indexOfTopLevelItem(item)
            self._sideView.workspacesList.takeTopLevelItem(index)
        self._sideView.workspacesList.blockSignals(False)

    def modifySpectrumLabel(self, wsName, spectrumIndex, label, color):
        """
        Modify spectrum label text and color.

        Args:
            wsName (str): name of the concerned workspace
            spectrumIndex (int): index of the spectrum
            label (str): new label
            color (str): new color (#RRGGBB)
        """
        wsItem = self._sideView.workspacesList.findItems(wsName,
                                                         Qt.MatchExactly, 0)
        if not wsItem:
            return
        wsItem = wsItem[0]

        for i in range(wsItem.childCount()):
            spItem = wsItem.child(i)
            if spItem.getSpectrumIndex() == spectrumIndex:
                brush = spItem.foreground(0)
                brush.setColor(QColor(color))
                spItem.setForeground(0, brush)
                spItem.setText(0, label)

    def setWorkspacesList(self, names):
        """
        Set the list of selected workspaces and update the workspace slider
        length.

        Args:
            names (list(str)): list if the workspace names
        """
        self._sideView.workspacesList.blockSignals(True)
        self._sideView.workspacesList.clear()
        for name in names:
            item = WorkspaceItem(self._sideView.workspacesList, name)
            item.signals.delClicked.connect(self._presenter.onDelButtonClicked)
            item.setText(0, name)
        self._sideView.workspacesList.blockSignals(False)

    def setSpectraList(self, name, nums):
        """
        Set the list of spectrum index for a workspace in the list.

        Args:
            name (str): name of the workspace
            nums (list(int)): list of the spectrum indexes
        """
        self._sideView.workspacesList.blockSignals(True)
        wsItem = self._sideView.workspacesList.findItems(name,
                                                         Qt.MatchExactly, 0)[0]
        wsItem.takeChildren()
        for num in nums:
            item = SpectrumItem(wsItem, num)
            item.signals.delClicked.connect(
                    self._presenter.onDelSpectrumButtonClicked)
            item.setText(0, str(num))
        self._sideView.workspacesList.blockSignals(False)

    def getSpectraList(self, name):
        """
        Get the list of spectrum indexes for a workspace in the list.

        Args:
            name (str): name of the workspace

        Returns:
            list(int): list of the spectrum indexes
        """
        wsItem = self._sideView.workspacesList.findItems(name, Qt.MatchExactly,
                                                         0)
        if wsItem:
            wsItem = wsItem[0]
        else:
            return []
        nums = list()
        for i in range(wsItem.childCount()):
            nums.append(wsItem.child(i).getSpectrumIndex())
        return nums

    def checkHoldButton(self, state):
        """
        Set the check state of the hold button. This function does not trigger
        the button signals.

        Args:
            state (bool): check state
        """
        self._bottomView.holdButton.blockSignals(True)
        self._bottomView.holdButton.setChecked(state)
        self._bottomView.holdButton.blockSignals(False)

    def setSpectrumDisabled(self, state):
        """
        Disable/enable the spectrum selection widgets (slider and spinbox).

        Args:
            state (bool): if True, disable the widgets
        """
        self._bottomView.spectrumSlider.setDisabled(state)
        self._bottomView.spectrumSpinBox.setDisabled(state)
        self._bottomView.holdButton.setDisabled(state)

    def setSpectrumSliderMax(self, length):
        """
        Set the max value of the spectrum slider.

        Args:
            value (int): slider maximum value
        """
        self._bottomView.spectrumSlider.setMaximum(length)

    def setSpectrumSliderPosition(self, position):
        """
        Set the spectrum slider position. This function does not trigger any
        QSlider signals.

        Args:
            position (int): position
        """
        self._bottomView.spectrumSlider.blockSignals(True)
        self._bottomView.spectrumSlider.setSliderPosition(position)
        self._bottomView.spectrumSlider.blockSignals(False)

    def getSpectrumSliderPosition(self):
        """
        Get the spectrum slider position.

        Returns:
            int: slider position
        """
        return self._bottomView.spectrumSlider.value()

    def setSpectrumSpinBoxMax(self, value):
        """
        Set the max value of the spectrum spinbox.

        Args:
            value (int): spinbox max value
        """
        self._bottomView.spectrumSpinBox.setMaximum(value)

    def setSpectrumSpinBoxValue(self, value):
        """
        Set the spectrum spinbox value. This function does not trigger any
        spinbox signals.

        Args:
            value (int): spinbox value
        """
        self._bottomView.spectrumSpinBox.blockSignals(True)
        self._bottomView.spectrumSpinBox.setValue(value)
        self._bottomView.spectrumSpinBox.blockSignals(False)

    def getSpectrumSpinBoxValue(self):
        """
        Get the current spectrum spinbox value.

        Returns:
            int: spinbox value
        """
        return self._bottomView.spectrumSpinBox.value()

    def setAvailableModes(self, modes):
        """
        Set available modes in the mode combobox.

        Args:
            modes (list(str)): list of modes
        """
        self._bottomView.modeComboBox.blockSignals(True)
        self._bottomView.modeComboBox.clear()
        self._bottomView.modeComboBox.addItems(modes)
        self._bottomView.modeComboBox.blockSignals(False)

    def setMode(self, mode):
        """
        Set the selected mode. This method does not trigegr any combobox signal.

        Args:
            mode (str): mode
        """
        self._bottomView.modeComboBox.blockSignals(True)
        self._bottomView.modeComboBox.setCurrentText(mode)
        self._bottomView.modeComboBox.blockSignals(False)

    def getMode(self):
        """
        Get the selected mode from the combobox.

        Returns:
            str: mode
        """
        return self._bottomView.modeComboBox.currentText()
