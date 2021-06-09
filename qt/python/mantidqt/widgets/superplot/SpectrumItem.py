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
    sig_del_clicked = Signal(str, int)

    def __init__(self):
        super().__init__()


class SpectrumItem(QTreeWidgetItem):

    """
    Index of the spectrum represented by this item.
    """
    _spectrum_index = None

    """
    Name of the corresponding workspace.
    """
    _workspace_name = None

    """
    Delete button.
    """
    _del_button = None

    """
    Spectrum item signals.
    """
    signals = None

    def __init__(self, treeWidget, index):
        super().__init__(treeWidget)
        self._spectrum_index = index
        self._workspace_name = self.parent().get_workspace_name()
        self._del_button = QToolButton()
        self._del_button.setMinimumSize(20, 20)
        self._del_button.setMaximumSize(20, 20)
        self._del_button.setText("-")
        self._del_button.setToolTip("Remove from the list")
        self.signals = SpectrumItemSignals()
        self._del_button.clicked.connect(
                 lambda c : self.signals.sig_del_clicked.emit(
                     self._workspace_name, self._spectrum_index))
        self.treeWidget().setItemWidget(self, 1, self._del_button)
        self.treeWidget().resizeColumnToContents(1)

    def get_spectrum_index(self):
        """
        Get the index of the spectrum.

        Returns:
            int: index of the spectrum
        """
        return self._spectrum_index
