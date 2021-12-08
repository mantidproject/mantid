# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from .view import RegionSelectorView
from ..observers.observing_presenter import ObservingPresenter


class RegionSelector(ObservingPresenter):

    def __init__(self, ws, parent=None, view=None):
        super().__init__()
        self.view = view if view else RegionSelectorView(self, parent)
