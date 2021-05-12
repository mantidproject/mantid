# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# local
from .model import PeakActionsModel

# 3rd party
from mantid.kernel import logger

# standard
import enum
from typing import Optional


class PeakActionsEvent(enum.Enum):
    ERASING_MODE_CHANGED = 1
    ADDING_MODE_CHANGED = 2


class PeakActionsPresenter:

    @staticmethod
    def _validate_view(view):
        r"""We require the actions view object to have certain attributes"""
        for attribute in ('collection_view', 'active_peaksworkspace_index', 'erasing_mode_on'):
            if hasattr(view, attribute) is False:
                raise TypeError(f'Attribute {attribute} not found in {view}')

    def __init__(self,
                 model: PeakActionsModel,
                 view: 'PeakActionsView'):
        PeakActionsPresenter._validate_view(view)
        self._model: PeakActionsModel = model
        self._model.presenter = self
        self._view: 'PeakActionsView' = view
        view.subscribe(self)  # subscribe to event notifications from the viewer

    @property
    def viewer_presenter(self) -> Optional['PeaksViewerPresenter']:
        r"""PeaksViewerPresenter associated to the active PeaksWorkspace"""
        index = self._view.active_peaksworkspace_index
        if index < 0:
            return None  # No peaksworkspace has yet been selected
        collection_view: 'PeaksViewerCollectionView' = self._view.collection_view
        peak_viewer: 'PeaksViewerView' = collection_view[index]
        return peak_viewer.presenter

    @property
    def active_peaksworkspace_index(self) -> int:
        return self._view.active_peaksworkspace_index

    def response_function(self, event: PeakActionsEvent):
        r"""
        Factory of response functions to signals emitted by the view (events)
        """
        def response_invalid():
            logger.error(f'{event} is an invalid PeakActionsEvent')
        responses = {PeakActionsEvent.ERASING_MODE_CHANGED: self._remove_peaks,
                     PeakActionsEvent.ADDING_MODE_CHANGED: self._add_peaks}
        return responses.get(event, response_invalid)

    def append_peaksworkspace(self, name: str):
        self._view.append_peaksworkspace(name)

    def remove_peaksworkspace(self, name: str):
        self._view.remove_peaksworkspace(name)

    def _remove_peaks(self):
        r"""Delete peaks if the button is pressed"""
        if self._view.erasing_mode_on:
            self._model.delete_peaks()  # delete peaks from the underlying workspace
            self.viewer_presenter.redraw_peaks()

    def _add_peaks(self):
        """Add peaks if the button is pressed"""
        self._view.collection_view._sliceinfo_provider.view.data_view.enable_peak_addition(self._view.adding_mode_on)
