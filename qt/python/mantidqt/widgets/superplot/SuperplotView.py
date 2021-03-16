# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotPresenter import SuperplotPresenter

from mantid.plots.plotfunctions import plot, get_plot_fig

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
    _canvas = None

    def __init__(self, canvas, parent=None):
        super().__init__(parent)
        self._canvas = canvas
        self._presenter = SuperplotPresenter(self)
        self._sideView = SuperplotViewSide(self)
        self._bottomView = SuperplotViewBottom(self)

        side = self._sideView
        side.addButton.clicked.connect(self._presenter.onAddButtonClicked)
        side.delButton.clicked.connect(self._presenter.onDelButtonClicked)
        bottom = self._bottomView
        bottom.holdButton.toggled.connect(self._presenter.onHoldButtonToggled)
        bottom.workspaceSlider.valueChanged.connect(self._presenter.onWorkspaceSliderMoved)
        bottom.spectrumSlider.valueChanged.connect(self._presenter.onSpectrumSliderMoved)

    def getSideWidget(self):
        return self._sideView

    def getBottomWidget(self):
        return self._bottomView

    def hide(self):
        super().hide()
        self._sideView.hide()
        self._bottomView.hide()

    def show(self):
        super().show()
        self._sideView.show()
        self._bottomView.show()

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
        self._bottomView.workspaceSlider.setMaximum(len(names) - 1)

    def getWorkspaceSliderPosition(self):
        """
        Get the workspace slider position.

        Returns:
            int: slider position
        """
        return self._bottomView.workspaceSlider.value()

    def setSpectrumSliderMax(self, length):
        """
        Set the max value of the spectrum slider.

        Args:
            value (int): slider maximum value
        """
        self._bottomView.spectrumSlider.setMaximum(length)

    def setSpectrumSliderPosition(self, position):
        """
        Set the spectrum slider position.

        Args:
            position (int): position
        """
        self._bottomView.spectrumSlider.setValue(position)

    def getSpectrumSliderPosition(self):
        """
        Get the spectrum slider position.

        Returns:
            int: slider position
        """
        return self._bottomView.spectrumSlider.value()

    def plotData(self, data):
        """
        Plot a list of workspace, spectrum pairs.

        Args:
            data (list(tuple(str, int))): list of ws, spectrum number pairs
        """
        figure, _ = get_plot_fig(fig=self._canvas.figure)
        for i in data:
            plot([i[0]], spectrum_nums=[i[1] + 1], overplot=True, fig=figure)
