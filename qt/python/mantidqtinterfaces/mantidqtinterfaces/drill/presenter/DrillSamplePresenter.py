# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .DrillParameterPresenter import DrillParameterPresenter
from .DrillParametersPresenter import DrillParametersPresenter


class DrillSamplePresenter:
    """
    Reference to the dril table.
    """

    _table = None

    """
    Reference to the sample model.
    """
    _sample = None

    CUSTOM_OPTIONS = "CustomOptions"

    # colors for the table rows
    OK_COLOR = "#3f00ff00"
    ERROR_COLOR = "#3fff0000"
    PROCESSING_COLOR = "#3fffff00"

    def __init__(self, table, sample):
        self._table = table
        self._sample = sample
        self._sample.groupChanged.connect(self.onGroupChanged)
        self._sample.newParameter.connect(self.onNewParameter)
        self._sample.statusChanged.connect(self.onStatusChanged)
        self._table.addSamplePresenter(self, self._sample.getIndex())

    def onNewItem(self, name, item):
        """
        Triggered when a new item is added in the table concerning this sample.
        This function create a MVP for the specific item.

        Args:
            name (str): name of the item (parameter name)
            item (QTableWidgetItem): new item
        """
        if name == self.CUSTOM_OPTIONS:
            DrillParametersPresenter(self._table, item, self._sample)
        else:
            self._sample.addParameter(name)
        self._table.setWindowModified(True)

    def onNewParameter(self, parameter):
        """
        Triggered when the sample receives a new parameter.

        Args:
            parameter (Drillparameter): the new parameter
        """
        item = self._table.itemFromName(self._sample.getIndex(), parameter.getName())
        if item is None:
            item = self._table.itemFromName(self._sample.getIndex(), self.CUSTOM_OPTIONS)
            presenter = item.getPresenter()
            if presenter is None:
                presenter = DrillParametersPresenter(self._table, item, self._sample)
            presenter.addParameter(parameter.getName())
        else:
            DrillParameterPresenter(item, self._sample, parameter)

    def onGroupChanged(self):
        """
        Triggered when the group of the sample changed.
        """
        group = self._sample.getGroup()
        index = self._sample.getIndex()
        if group is None:
            self._table.delRowLabel(index)
        else:
            groupName = group.getName()
            isMaster = group.getMaster() == self._sample
            groupIndex = group.getSampleIndex(self._sample)
            self._table.setRowLabel(index, groupName + str(groupIndex + 1), bold=isMaster)
        self._table.setWindowModified(True)

    def onStatusChanged(self):
        """
        Triggered when the status of the sample changed. This function
        colors the associated row in accordance to this status.
        """
        index = self._sample.getIndex()
        status = self._sample.getStatus()
        if status is None:
            self._table.removeRowBackground(index)
        elif status == self._sample.STATUS_PROCESSED:
            self._table.setRowBackground(index, self.OK_COLOR)
        elif status == self._sample.STATUS_ERROR:
            self._table.setRowBackground(index, self.ERROR_COLOR)
        elif status == self._sample.STATUS_PENDING:
            self._table.setRowBackground(index, self.PROCESSING_COLOR)
