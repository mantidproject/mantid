# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .DrillParameterPresenter import DrillParameterPresenter


class DrillSamplePresenter:

    """
    Reference to the dril table.
    """
    _table = None

    """
    Reference to the sample model.
    """
    _sample = None

    # colors for the table rows
    OK_COLOR = "#3f00ff00"
    ERROR_COLOR = "#3fff0000"
    PROCESSING_COLOR = "#3fffff00"

    def __init__(self, table, sample):
        self._table = table
        self._sample = sample
        self._sample.processStarted.connect(self.onProcessStarted)
        self._sample.processDone.connect(self.onProcessDone)
        self._table.addSamplePresenter(self, self._sample.getIndex())

    def onNewItem(self, name, item):
        """
        Triggered when a new item is added in the table concerning this sample.

        Args:
            name (str): name of the item (parameter name)
            item (QTableWidgetItem): new item
        """
        parameter = self._sample.addParameter(name)
        presenter = DrillParameterPresenter(item, parameter)

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
