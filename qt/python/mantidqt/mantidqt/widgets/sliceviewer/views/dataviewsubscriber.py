# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABC, abstractmethod


class IDataViewSubscriber(ABC):
    @abstractmethod
    def dimensions_changed(self) -> None:
        pass

    @abstractmethod
    def slicepoint_changed(self) -> None:
        pass

    @abstractmethod
    def mpl_button_clicked(self, event) -> None:
        pass

    @abstractmethod
    def zoom_pan_clicked(self, active) -> None:
        pass
