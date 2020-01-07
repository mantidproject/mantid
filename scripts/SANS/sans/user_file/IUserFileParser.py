# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod

from sans.common.enums import SANSInstrument
from sans.user_file.parsed_containers.BackgroundDetails import BackgroundDetails
from sans.user_file.parsed_containers.DetectorDetails import DetectorDetails
from sans.user_file.parsed_containers.FitDetails import FitDetails
from sans.user_file.parsed_containers.LimitDetails import LimitDetails
from sans.user_file.parsed_containers.MaskDetails import MaskDetails
from sans.user_file.parsed_containers.MonitorDetails import MonitorDetails
from sans.user_file.parsed_containers.QResolutionDetails import QResolutionDetails
from sans.user_file.parsed_containers.SampleDetails import SampleDetails
from sans.user_file.parsed_containers.SetPositionDetails import SetPositionDetails
from sans.user_file.parsed_containers.TransmissionDetails import TransmissionDetails


class IUserFileParser(ABC):

    @abstractmethod
    def get_background_details(self) -> BackgroundDetails:
        pass

    @abstractmethod
    def get_detector_details(self) -> DetectorDetails:
        pass

    @abstractmethod
    def get_fit_details(self) -> FitDetails:
        pass

    @abstractmethod
    def get_gravity_details(self) -> (bool, float):
        """
        :return: Bool - True / False for gravity corrections, float - The extra length in m to correct for
        """
        pass

    @abstractmethod
    def get_instrument(self) -> SANSInstrument:
        pass

    @abstractmethod
    def get_limit_details(self) -> LimitDetails:
        pass

    @abstractmethod
    def get_mask_details(self) -> MaskDetails:
        pass

    @abstractmethod
    def get_monitor_details(self) -> MonitorDetails:
        pass

    @abstractmethod
    def get_q_resolution_details(self) -> QResolutionDetails:
        pass

    @abstractmethod
    def get_sample_details(self) -> SampleDetails:
        pass

    @abstractmethod
    def get_set_position_details(self) -> SetPositionDetails:
        pass

    @abstractmethod
    def get_transmission_details(self) -> TransmissionDetails:
        pass

    @abstractmethod
    def get_tube_calibration_filename(self) -> str:
        pass

    @abstractmethod
    def is_compatibility_mode_on(self) -> bool:
        pass
