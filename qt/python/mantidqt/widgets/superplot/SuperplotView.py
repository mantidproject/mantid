# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotPresenter import SuperplotPresenter

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

    def __init__(self, parent=None):
        super().__init__(parent)
        self._presenter = SuperplotPresenter(self)
        self._sideView = SuperplotViewSide(self)
        self._bottomView = SuperplotViewBottom(self)

        side = self._sideView
        side.addButton.clicked.connect(self._presenter.onAddButtonClicked)
        bottom = self._bottomView
        bottom.holdButton.toggled.connect(self._presenter.onHoldButtonToggled)

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
