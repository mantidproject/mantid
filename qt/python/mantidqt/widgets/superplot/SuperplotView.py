# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QDockWidget, QWidget
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
        side.workspacesList.currentRowChanged.connect(
                self._presenter.onWorkspaceSelectionChanged)
        bottom = self._bottomView
        bottom.holdButton.toggled.connect(self._presenter.onHoldButtonToggled)
        bottom.workspaceSlider.valueChanged.connect(
                self._presenter.onWorkspaceSliderMoved)
        bottom.workspaceSpinBox.valueChanged.connect(
                self._presenter.onWorkspaceSpinBoxChanged)
        bottom.spectrumSlider.valueChanged.connect(
                self._presenter.onSpectrumSliderMoved)
        bottom.spectrumSpinBox.valueChanged.connect(
                self._presenter.onSpectrumSpinBoxChanged)

    def getSideWidget(self):
        return self._sideView

    def getBottomWidget(self):
        return self._bottomView

    def close(self):
        self._sideView.close()
        self._bottomView.close()

    def setSelectedWorkspace(self, index):
        """
        Select a specific workspace in the workspace list. This function does
        not raise any QList signals.

        Args:
            index (int): new selection index
        """
        self._sideView.workspacesList.blockSignals(True)
        self._sideView.workspacesList.setCurrentRow(index - 1)
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
            return item.text()
        else:
            return None

    def setWorkspacesList(self, names):
        """
        Set the list of selected workspaces and update the workspace slider
        length.

        Args:
            names (list(str)): list if the workspace names
        """
        self._sideView.workspacesList.clear()
        self._sideView.workspacesList.addItems(names)
        self._bottomView.workspaceSlider.setMaximum(len(names))
        self._bottomView.workspaceSpinBox.setMaximum(len(names))
        self._sideView.workspacesList.setCurrentRow(len(names) - 1)

    def setWorkspaceSliderPosition(self, position):
        """
        Set the workspace slider position. This function does not trigger the
        slider signals.

        Args:
            position (int): new slider position
        """
        self._bottomView.workspaceSlider.blockSignals(True)
        self._bottomView.workspaceSlider.setSliderPosition(position)
        self._bottomView.workspaceSlider.blockSignals(False)

    def getWorkspaceSliderPosition(self):
        """
        Get the workspace slider position.

        Returns:
            int: slider position
        """
        return self._bottomView.workspaceSlider.value()

    def setWorkspaceSpinBoxValue(self, value):
        """
        Set the workspace spinbox value. This function does not trigger the
        spinbox signals.

        Args:
            value (int): new value
        """
        self._bottomView.workspaceSpinBox.blockSignals(True)
        self._bottomView.workspaceSpinBox.setValue(value)
        self._bottomView.workspaceSpinBox.blockSignals(False)

    def getWorkspaceSpinBoxValue(self):
        """
        Get the workspace spinbox value.

        Returns:
            int: value
        """
        return self._bottomView.workspaceSpinBox.value()

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
