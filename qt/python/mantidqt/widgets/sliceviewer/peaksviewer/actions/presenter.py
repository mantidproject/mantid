# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# local
from ..presenter import PeaksViewerPresenter
from ..view import PeaksViewerView, PeaksViewerCollectionView
from .model import PeakActionsModel
from .view import PeakActionsView

# 3rd party
from mantid.kernel import logger

# standard
import enum
from typing import Optional


class PeakActionsEvent(enum.Enum):
    ACTIVE_WORKSPACE_CHANGED = 1
    ERASER_BUTTON_CHANGED = 2
    INSERTER_BUTTON_CHANGED = 3


class PeakActionsPresenter:

    @staticmethod
    def _validate_view(view):
        r"""We require the actions view object to have certain attributes"""
        for attribute in ('collection_viewer', 'selected_table_index', 'erasing_mode_on'):
            if getattr(view, attribute) is False:
                raise TypeError(f'{view} is not a valid PeakActionsView')

    def __init__(self,
                 model: PeakActionsModel,
                 view: PeakActionsView):
        PeakActionsPresenter._validate_view(view)
        self._model: PeakActionsModel = model
        self._view: PeakActionsView = view
        view.subscribe(self)  # subscribe to event notifications from the viewer

    @property
    def viewer_presenter(self) -> PeaksViewerPresenter:
        r"""PeaksViewerPresenter associated to the active PeaksWorkspace"""
        table_index = self._view.selected_table_index
        collection_viewer: PeaksViewerCollectionView = self._view.collection_viewer
        peak_viewer: PeaksViewerView = collection_viewer[table_index]
        return peak_viewer.presenter

    def notified(self, event: PeakActionsEvent):
        r"""
        Notification of an event by the viewer, that the presenter should react to
        """
        def response_invalid():
            logger.error(f'{event} is an invalid PeakActionsEvent')
        responses = {'ACTIVE_WORKSPACE_CHANGED': self._update_viewer_model_active,
                     'ERASER_BUTTON_CHANGED': self._delete_peaks}
        response = responses.get(event, response_invalid)
        response()

    def _update_viewer_model_active(self) -> None:
        r"""Update the active PeaksViewerModel cached in the PeakActionsModel"""
        self._model.viewer_model = self.viewer_presenter.model

    def _delete_peaks(self):
        r"""Delete peaks if the button is pressed"""
        if self._view.erasing_mode_on:
            self._model.delete_peaks()
            self.viewer_presenter.redraw_peaks()
