# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABC, abstractmethod


class IDataViewSubscriber(ABC):
    @abstractmethod
    def canvas_clicked(self, event) -> None:
        pass

    @abstractmethod
    def data_limits_changed(self):
        pass

    @abstractmethod
    def dimensions_changed(self) -> None:
        pass

    @abstractmethod
    def line_plots(self, state: bool) -> None:
        pass

    @abstractmethod
    def nonorthogonal_axes(self, state: bool) -> None:
        pass

    @abstractmethod
    def region_selection(self, state: bool) -> None:
        pass

    @abstractmethod
    def show_all_data_clicked(self) -> None:
        pass

    @abstractmethod
    def slicepoint_changed(self) -> None:
        pass

    @abstractmethod
    def zoom_pan_clicked(self, active) -> None:
        pass
