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

    def __init__(self, table, sample):
        self._table = table
        self._sample = sample

    def onNewItem(self, name, item):
        """
        Triggered when a new item is added in the table concerning this sample.

        Args:
            name (str): name of the item (parameter name)
            item (QTableWidgetItem): new item
        """
        parameter = self._sample.addParameter(name)
        presenter = DrillParameterPresenter(item, parameter)
