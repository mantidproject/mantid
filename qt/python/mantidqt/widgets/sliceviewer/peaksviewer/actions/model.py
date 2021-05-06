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
        self._viewer_model: Optional[PeaksViewerModel] = None

    @property
    def viewer_model(self):
        return self._viewer_model

    @viewer_model
    def viewer_model(self, model: PeaksViewerModel):
        self._viewer_model = model

    def delete_peaks(self):
        r"""Delete the first peak of the model"""
        self._viewer_model.delete_rows(0)
