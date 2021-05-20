# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# 3rd party
from qtpy import QtWidgets
from mantidqt.utils.qt import load_ui

# standard
from typing import Optional


class PeakActionsView(QtWidgets.QWidget):
    def __init__(self, parent: Optional['PeaksViewerCollectionView'] = None):
        super(PeakActionsView, self).__init__(parent=parent)
        self._presenter: 'PeakActionsPresenter' = None
        self.ui = None
        self._setup_ui()

    @property
    def presenter(self) -> 'PeakActionsPresenter':
        return self._presenter

    def subscribe(self, presenter: 'PeakActionsPresenter') -> None:
        r"""
        @brief Subscribe a presenter to the viever
        @details The presenter must have method 'response_function' able to handle the event
        """
        self._presenter = presenter
        self._route_signals_to_presenter()

    @property
    def erasing_mode_on(self):
        r"""Find if the button to remove peaks is checked"""
        return self.ui.remove_peaks_button.isChecked()

    @property
    def adding_mode_on(self):
        r"""Find if the button to add peaks is checked"""
        return self.ui.add_peaks_button.isChecked()

    @property
    def active_peaksworkspace_index(self):
        r"""Find index of the currently selected PeaksWorkspace. Returns -1 is nothing is selected"""
        return self.ui.active_peaks_combobox.currentIndex()

    def deactivate_peak_adding(self):
        self.ui.add_peaks_button.setChecked(False)
        self.ui.remove_peaks_button.setChecked(False)

    def append_peaksworkspace(self, name: str) -> None:
        self.ui.active_peaks_combobox.addItem(name)

    def remove_peaksworkspace(self, name: str):
        box = self.ui.active_peaks_combobox
        index = box.findText(name)
        box.removeItem(index)

    def _setup_ui(self):
        self.ui = load_ui(__file__, 'actions.ui', self)
        # Styling
        self.ui.add_peaks_button.setStyleSheet("background-color:lightgrey")
        self.ui.remove_peaks_button.setStyleSheet("background-color:lightgrey")

        self.ui.add_peaks_button.clicked.connect(self._add_button_clicked)
        self.ui.remove_peaks_button.clicked.connect(self._delete_button_clicked)

    def _route_signals_to_presenter(self):
        r"""Link viewer particular viewer signals to particular methods of the presenter"""
        self.ui.add_peaks_button.clicked.connect(self.presenter.deactivate_zoom_pan)
        self.ui.remove_peaks_button.clicked.connect(self.presenter.deactivate_zoom_pan)

    def _add_button_clicked(self, state):
        if state:
            self.ui.remove_peaks_button.setChecked(False)

    def _delete_button_clicked(self, state):
        if state:
            self.ui.add_peaks_button.setChecked(False)
