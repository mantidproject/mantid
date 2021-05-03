# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# local
from ..presenter import PeaksViewerPresenter, PeaksViewerCollectionPresenter
from .model import PeakActionsModel
from .view import PeakActionsView

# standard
import enum
from typing import Optional


class PeakActionsEvent(enum.Enum):
    ACTIVE_WORKSPACE_CHANGED = 1
    ERASE_PEAK_CHANGED = 2
    ADD_PEAK_CHANGED = 3


class PeakActionsPresenter:

    def __init__(self,
                 model: PeakActionsModel,
                 view: PeakActionsView):
        self._model: PeakActionsModel = model
        self._view: PeakActionsView = view
        self._collection_presenter: Optional[PeaksViewerCollectionPresenter] = None

    @property
    def active_presenter(self) -> PeaksViewerPresenter:
        table_index = self._view.selected_table_index
        return self._collection_presenter.child_presenter(table_index)

