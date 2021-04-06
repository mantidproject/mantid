# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QTreeWidgetItem


class SpectrumItem(QTreeWidgetItem):

    """
    Index of the spectrum represented by this item.
    """
    _spectrumIndex = None

    def __init__(self, treeWidget, index):
        super().__init__(treeWidget)
        self._spectrumIndex = index

    def getSpectrumIndex(self):
        """
        Get the index of the spectrum.

        Returns:
            int: index of the spectrum
        """
        return self._spectrumIndex
