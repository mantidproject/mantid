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
from mantidqt.utils.qt import load_ui

# local
from ..view import PeaksViewerCollectionView
from .presenter import PeakActionsEvent, PeakActionsPresenter

# standard
from typing import Optional


class NotifierFactory:
    r"""
    @brief Factory of functions that are invoked when the widgets of the view emit a signal
    @details The function is a wrapper to one of the methods of the PeakActionsPresenter object aggregated to the view
    """

    def __init__(self,
                 presenter: PeakActionsPresenter,
                 event: PeakActionsEvent) -> None:
        self._presenter = presenter
        self._event = event

    def __call__(self):
        r"""Instances of this class can be called, thus behaving as functions"""
        self._presenter.notified(self._event)


class PeakActionsView(QtWidgets.QWidget):
    def __init__(self, parent: Optional[PeaksViewerCollectionView] = None):
        super(PeakActionsView, self).__init__(parent=parent)
        self._collection_view = parent
        self._presenter: 'PeakActionsPresenter' = None
        self._setup_ui()

    @property
    def collection_view(self) -> PeaksViewerCollectionView:
        return self._collection_view

    @property
    def presenter(self) -> PeakActionsPresenter:
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

    @property
    def erasing_mode_on(self):
        r"""Find if the button to remove peaks is checked"""
        return self.ui.remove_peaks_button.isChecked()

    @property
    def active_peaksworkspace_index(self):
        r"""Find index of the currently selected PeaksWorkspace. Returns -1 is nothing is selected"""
        return self.ui.active_peaks_combobox.currentIndex()

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
        # Associate a QSignal to a response function of the presenter
        signal_translator = [(self.ui.remove_peaks_button.clicked, PeakActionsEvent.ERASING_MODE_CHANGED),
                             (self.ui.add_peaks_button.clicked, PeakActionsEvent.ADDING_MODE_CHANGED)]
        # Link the QSignal to a method of the presenter
        for qsignal, event in signal_translator:
            qsignal.connect(self._presenter.response_function(event))
