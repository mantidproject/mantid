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
from typing import Annotated, Optional, TypeAlias

# Forward declarations
PeaksViewerCollectionPresenter: TypeAlias = Annotated[type, "PeaksViewerCollectionPresenter"]
PeaksViewerCollectionView: TypeAlias = Annotated[type, "PeaksViewerCollectionView"]


class PeakActionsView(QtWidgets.QWidget):
    def __init__(self, parent: Optional[PeaksViewerCollectionView] = None):
        super(PeakActionsView, self).__init__(parent=parent)
        self._presenter: PeaksViewerCollectionPresenter = None
        self.ui = None
        self._setup_ui()

    @property
    def presenter(self) -> PeaksViewerCollectionPresenter:
        return self._presenter

    def subscribe(self, presenter: PeaksViewerCollectionPresenter) -> None:
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
    def active_peaksworkspace(self):
        r"""Return the currently selected PeaksWorkspace."""
        return self.ui.active_peaks_combobox.currentText()

    def deactivate_peak_adding(self):
        self.ui.add_peaks_button.setChecked(False)
        self.ui.remove_peaks_button.setChecked(False)

    def set_peaksworkspace(self, names):
        """Set the items in the combobox.

        The names are sorted to prevent reordering when the workspace
        is replaced in the ADS after adding or removing a peak.

        The current name set back after we replace all the items.
        """
        current_name = self.ui.active_peaks_combobox.currentText()
        self.ui.active_peaks_combobox.clear()
        self.ui.active_peaks_combobox.addItems(sorted(names))
        self.ui.active_peaks_combobox.setCurrentText(current_name)

    def _setup_ui(self):
        self.ui = load_ui(__file__, "actions.ui", self)
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
