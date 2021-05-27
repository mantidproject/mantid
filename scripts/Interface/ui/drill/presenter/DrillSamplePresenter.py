# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

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
