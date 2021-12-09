# Mantid Repository : https://github.com/mantidproject/mantid#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from .view import RegionSelectorView
from ..observers.observing_presenter import ObservingPresenter
from ..sliceviewer.models.dimensions import Dimensions
from ..sliceviewer.views.dataviewsubscriber import IDataViewSubscriber


class RegionSelector(ObservingPresenter, IDataViewSubscriber):
    def __init__(self, ws, parent=None, view=None):
        super().__init__()
        self._dimensions = Dimensions(ws)
        self.view = view if view else RegionSelectorView(self, parent, dims_info=self._dimensions.get_dimensions_info())

    def dimensions_changed(self) -> None:
        pass

    def slicepoint_changed(self) -> None:
        pass

    def mpl_button_clicked(self, event) -> None:
        pass

    def zoom_pan_clicked(self, active) -> None:
        pass
