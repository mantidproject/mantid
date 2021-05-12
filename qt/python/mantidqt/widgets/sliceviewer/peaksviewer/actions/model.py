# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

# local
from ..model import PeaksViewerModel

# standard
from typing import Optional


class PeakActionsModel:
    def __init__(self):
        self.presenter: Optional['PeakActionsPresenter'] = None

    @property
    def viewer_model(self) -> Optional[PeaksViewerModel]:
        r"""current model subject to potential peak actions"""
        viewer_presenter = self.presenter.viewer_presenter
        return viewer_presenter.model

    def delete_peaks(self):
        r"""Delete the first peak of the model"""
        self.viewer_model.delete_rows(0)
