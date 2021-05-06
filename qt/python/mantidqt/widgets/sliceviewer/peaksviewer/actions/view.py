# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# 3rd party
from qtpy import QtWidgets, QtGui
from mantid.kernel import logger

# standard
from typing import List


class PeakActionsView:
    def __init__(self):
        self._presenter: 'PeakActionsPresenter' = None
        peak_grid = QtGui.QGridLayout()

        self.add_btn = QtWidgets.QPushButton('Add peak', self)
        self.rm_btn = QtWidgets.QPushButton('Remove peak', self)
        self.peaks_menu = QtWidgets.QComboBox()

        wksp_list = ["Workspace 1", "Workspace 2"]
        self.peaks_menu.addItems(wksp_list)

        self.add_btn.setStyleSheet("background-color:lightgrey")
        self.rm_btn.setStyleSheet("background-color:lightgrey")

        self.add_btn.clicked.connect(self.add_btn_click)
        self.rm_btn.clicked.connect(self.rm_btn_click)

        peak_grid.addWidget(self.add_btn, 1, 2)
        peak_grid.addWidget(self.rm_btn, 2, 2)
        peak_grid.addWidget(self.peaks_menu, 1, 1)

        self.setLayout(peak_grid)

    @property
    def presenter(self):
        return self._presenter

    def subscribe(self, presenter: 'PeakActionsPresenter') -> None:
        r"""
        @brief Subscribe a presenter to the viever
        @details The presenter must have method 'notified' able to handle the event
        """
        if getattr(presenter, 'notified') is False:
            logger.error(f'{presenter} lacks method "notified"')
        else:
            self._presenter = presenter

    def _notify(self, event: 'PeakActionsEvent'):
        r"""Notify of a PeakActionsEvent to the subscribers"""
        self._presenter.notified(event)

    def add_btn_click(self):
        pass  # self._notify(PeakActionsEvent.ACTIVE_WORKSPACE_CHANGED) or some other event more appropriate

    def rm_btn_click(self):
        pass
