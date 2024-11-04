# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTableWidgetItem
from qtpy.QtCore import QObject, Signal, Qt


class DrillTableItemSignals(QObject):
    dataChanged = Signal()


class DrillTableItem(QTableWidgetItem):
    """
    Reference to the item presenter.
    """

    _presenter = None

    signals = None

    def __init__(self):
        super().__init__(QTableWidgetItem.UserType)
        self.signals = DrillTableItemSignals()

    def clone(self):
        """
        Override QTableWidgetItem::clone.

        Return:
            DrillTableItem: a new item
        """
        return DrillTableItem()

    def setData(self, role, value):
        """
        Override QTableWidgetItem::setData.
        """
        super().setData(role, value)
        if role == Qt.EditRole:
            self.signals.dataChanged.emit()

    def setPresenter(self, presenter):
        """
        Set the presenter.

        Args:
            presenter (DrillParameterPresenter): the presenter
        """
        self._presenter = presenter

    def getPresenter(self):
        """
        Get the presenter.

        Returns:
            DrillParameterPresenter: the presenter
        """
        return self._presenter
