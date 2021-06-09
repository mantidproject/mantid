# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QTreeWidgetItem, QToolButton
from qtpy.QtCore import QObject, Signal


class SpectrumItemSignals(QObject):

    """
    Thrown when the delete button is pressed.
    Args:
        str: workspace name
        int: spectrum index
    """
    delClicked = Signal(str, int)

    def __init__(self):
        super().__init__()


class SpectrumItem(QTreeWidgetItem):

    """
    Index of the spectrum represented by this item.
    """
    _spectrumIndex = None

    """
    Name of the corresponding workspace.
    """
    _workspaceName = None

    """
    Delete button.
    """
    _delButton = None

    """
    Spectrum item signals.
    """
    signals = None

    def __init__(self, treeWidget, index):
        super().__init__(treeWidget)
        self._spectrumIndex = index
        self._workspaceName = self.parent().getWorkspaceName()
        self._delButton = QToolButton()
        self._delButton.setMinimumSize(20, 20)
        self._delButton.setMaximumSize(20, 20)
        self._delButton.setText("-")
        self._delButton.setToolTip("Remove from the list")
        self.signals = SpectrumItemSignals()
        self._delButton.clicked.connect(
                 lambda c : self.signals.delClicked.emit(self._workspaceName,
                                                         self._spectrumIndex))
        self.treeWidget().setItemWidget(self, 1, self._delButton)
        self.treeWidget().resizeColumnToContents(1)

    def get_spectrum_index(self):
        """
        Get the index of the spectrum.

        Returns:
            int: index of the spectrum
        """
        return self._spectrumIndex
