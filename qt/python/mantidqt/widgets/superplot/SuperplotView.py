# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QDockWidget, QWidget, QTreeWidgetItem
from qtpy.QtCore import *
from qtpy import uic

import os


class SuperplotViewSide(QDockWidget):

    UI = "ui/superplotSide.ui"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewSide.UI), self)


class SuperplotViewBottom(QDockWidget):

    UI = "ui/superplotBottom.ui"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewBottom.UI), self)


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
        side.visibilityChanged.connect(self._presenter.onVisibilityChanged)
        side.addButton.clicked.connect(self._presenter.onAddButtonClicked)
        side.delButton.clicked.connect(self._presenter.onDelButtonClicked)
        side.workspacesList.itemSelectionChanged.connect(
                self._presenter.onWorkspaceSelectionChanged)
        bottom = self._bottomView
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

    def setSelectedWorkspace(self, name):
        """
        Select a specific workspace in the workspace list. This function does
        not raise any QList signals.

        Args:
            index (int): new selection index
        """
        self._sideView.workspacesList.blockSignals(True)

        item = self._sideView.workspacesList.findItems(name, Qt.MatchExactly, 0)[0]
        if item:
            self._sideView.workspacesList.setCurrentItem(item)
        self._sideView.workspacesList.blockSignals(False)

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
            return item.text(0)
        else:
            return None

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
            if item.text(0) not in selection:
                selection.append(item.text(0))
        return selection

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
                wsName = item.text(0)
                if wsName not in selection:
                    selection[wsName] = list()
            else:
                spectrum = int(item.text(0))
                wsName = item.parent().text(0)
                if wsName in selection:
                    selection[wsName].append(spectrum)
                else:
                    selection[wsName] = [spectrum]
        return selection

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
            item = QTreeWidgetItem(self._sideView.workspacesList)
            item.setText(0, name)
        self._sideView.workspacesList.blockSignals(False)

    def setSpectraList(self, name, nums):
        """
        Set the list of spectrum index for a workspace in the list.

        Args:
            name (str): name of the workspace
            nums (list(int)): list of the spectrum indexes
        """
        wsItem = self._sideView.workspacesList.findItems(name,
                                                         Qt.MatchExactly, 0)[0]
        wsItem.takeChildren()
        for num in nums:
            item = QTreeWidgetItem(wsItem)
            item.setText(0, str(num))

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
            nums.append(int(wsItem.child(i).text(0)))
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
