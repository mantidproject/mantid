# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod

from sans.common.enums import SANSFacility
from sans.state.AllStates import AllStates
from sans.state.StateObjects.StateAdjustment import StateAdjustment
from sans.state.StateObjects.StateCalculateTransmission import StateCalculateTransmission
from sans.state.StateObjects.StateCompatibility import StateCompatibility
from sans.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans.state.StateObjects.StateData import StateData, get_data_builder
from sans.state.StateObjects.StateMaskDetectors import StateMaskDetectors
from sans.state.StateObjects.StateMoveDetectors import StateMove
from sans.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor
from sans.state.StateObjects.StateReductionMode import StateReductionMode
from sans.state.StateObjects.StateSave import StateSave
from sans.state.StateObjects.StateScale import StateScale
from sans.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans.state.StateObjects.StateWavelength import StateWavelength
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment
from sans.state.StateObjects.state_instrument_info import StateInstrumentInfo


class IStateParser(metaclass=ABCMeta):
    def get_all_states(self, file_information) -> AllStates:
        all_states = AllStates()
        all_states.move = self.get_state_move(file_information)
        all_states.reduction = self.get_state_reduction_mode()
        all_states.slice = self.get_state_slice_event()
        all_states.mask = self.get_state_mask(file_information)
        all_states.wavelength = self.get_state_wavelength()
        all_states.save = self.get_state_save()
        all_states.scale = self.get_state_scale(file_information)
        all_states.adjustment = self.get_state_adjustment(file_information)
        all_states.convert_to_q = self.get_state_convert_to_q()
        all_states.compatibility = self.get_state_compatibility()
        all_states.data = self.get_state_data(file_information)
        all_states.instrument_info = StateInstrumentInfo.build_from_data_info(all_states.data)

        return all_states

    @abstractmethod
    def get_state_adjustment(self, file_information) -> StateAdjustment:
        pass

    @abstractmethod
    def get_state_calculate_transmission(self) -> StateCalculateTransmission:
        pass

    @abstractmethod
    def get_state_compatibility(self) -> StateCompatibility:
        pass

    @abstractmethod
    def get_state_convert_to_q(self) -> StateConvertToQ:
        pass

    def get_state_data(self, file_information):
        if file_information:
            data_builder = get_data_builder(SANSFacility.ISIS, file_information)
            return data_builder.build()
        else:
            return StateData()

    @abstractmethod
    def get_state_mask(self, file_information) -> StateMaskDetectors:
        pass

    @abstractmethod
    def get_state_move(self, file_information) -> StateMove:
        pass

    @abstractmethod
    def get_state_normalize_to_monitor(self, file_information) -> StateNormalizeToMonitor:
        pass

    @abstractmethod
    def get_state_reduction_mode(self) -> StateReductionMode:
        pass

    @abstractmethod
    def get_state_save(self) -> StateSave:
        pass

    @abstractmethod
    def get_state_scale(self, file_information) -> StateScale:
        pass

    @abstractmethod
    def get_state_slice_event(self)  -> StateSliceEvent:
        pass

    @abstractmethod
    def get_state_wavelength(self)  -> StateWavelength:
        pass

    @abstractmethod
    def get_state_wavelength_and_pixel_adjustment(self) -> StateWavelengthAndPixelAdjustment:
        pass
