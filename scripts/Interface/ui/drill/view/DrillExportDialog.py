# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QDialog, QCheckBox, QToolButton
from qtpy.QtCore import *
from qtpy import uic

from mantidqt import icons
from mantidqt.interfacemanager import InterfaceManager


class DrillExportDialog(QDialog):

    UI_FILENAME = "ui/export.ui"

    """
    Export presenter.
    """
    _presenter = None

    """
    Dictionnary of algorithm names and their corresponding QCheckBox.
    """
    _widgets = None

    def __init__(self, parent=None):
        """
        Create the export dialog.

        Args:
            parent (QWidget): parent widget
        """
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, self.UI_FILENAME), self)
        self.okButton.clicked.connect(self.accept)
        self.cancelButton.clicked.connect(self.reject)
        self.applyButton.clicked.connect(
                lambda : self.accepted.emit()
                )
        self._widgets = dict()

    def setPresenter(self, presenter):
        """
        Set the prsesenter. This is needed to keep a valid reference to the
        presenter and avoid destruction by the  garage collector.

        Args:
            presenter (DrillExportPresenter): export presenter
        """
        self._presenter = presenter

    def setAlgorithms(self, algorithms):
        """
        Set the algorithms displayed on the dialog.

        Args:
            algorithms (list(str)): list of algorithms
        """
        for i in range(len(algorithms)):
            widget = QCheckBox(algorithms[i], self)
            self._widgets[algorithms[i]] = widget
            self.algoList.addWidget(widget, i, 0, Qt.AlignLeft)
            helpButton = QToolButton(self)
            helpButton.setText('...')
            helpButton.setIcon(icons.get_icon("mdi.help"))
            helpButton.clicked.connect(
                    lambda _, a=algorithms[i]: InterfaceManager().showHelpPage(
                        "qthelp://org.mantidproject/doc/algorithms/{}.html"
                        .format(a)))
            self.algoList.addWidget(helpButton, i, 1, Qt.AlignRight)

    def setAlgorithmCheckStates(self, states):
        """
        Set the check state of algorithm.

        Args:
            sates (dict(str:bool)): for each algorithm name, a bool to set its
                                    check state
        """
        for a,s in states.items():
            if a in self._widgets:
                self._widgets[a].setChecked(s)

    def getAlgorithmCheckStates(self):
        """
        Get the check state of algorithms.

        Returns:
            dict(str:bool): for each algorithm name, a bool to set its check
                            state
        """
        return {a:w.isChecked() for a,w in self._widgets.items()}
