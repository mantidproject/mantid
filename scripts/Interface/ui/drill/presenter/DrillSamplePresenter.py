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
        self._sample.processStarted.connect(self.onProcessStarted)
        self._sample.processDone.connect(self.onProcessDone)
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
            presenter = DrillParametersPresenter(item, self._sample)
        else:
            parameter = self._sample.addParameter(name)
            parameter.setValue(item.text())

    def onDelItem(self, name):
        """
        Triggered when an item is cleared.

        Args:
            name (str): name of the item (parameter name)
        """
        self._sample.delParameter(name)

    def onNewParameter(self, parameter):
        """
        Triggered when the sample receives a new parameter.

        Args:
            parameter (Drillparameter): the new parameter
        """
        item = self._table.itemFromName(self._sample.getIndex(),
                                        parameter.getName())
        if item is None:
            item = self._table.itemFromName(self._sample.getIndex(),
                                            self.CUSTOM_OPTIONS)
            presenter = item.getPresenter()
            if presenter is None:
                presenter = DrillParametersPresenter(item, self._sample)
            presenter.addParameter(parameter.getName())
        else:
            DrillParameterPresenter(item, parameter)

    def onGroupChanged(self):
        """
        Triggered when the group of the sample changed.
        """
        groupName = self._sample.getGroupName()
        index = self._sample.getIndex()
        if groupName is None:
            self._table.delRowLabel(index)
        else:
            isMaster = self._sample.isMaster()
            groupIndex = self._sample.getGroupIndex()
            self._table.setRowLabel(index, groupName + str(groupIndex),
                                    bold=isMaster)

    def onProcessStarted(self):
        """
        Triggered when the sample processing started.
        """
        index = self._sample.getIndex()
        self._table.setRowBackground(index, self.PROCESSING_COLOR)

    def onProcessDone(self, code):
        """
        Triggered when the sample processing ended.

        Args:
            code (int): return code. 0: success, -1: error
        """
        index = self._sample.getIndex()
        if code == 0:
            self._table.setRowBackground(index, self.OK_COLOR)
        else:
            self._table.setRowBackground(index, self.ERROR_COLOR)
