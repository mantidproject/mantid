# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""A Test director"""

from sans_core.state.StateObjects.StateData import get_data_builder
from sans_core.state.StateObjects.StateMoveDetectors import get_move_builder
from sans_core.state.StateObjects.StateReductionMode import StateReductionMode
from sans_core.state.StateObjects.StateSliceEvent import get_slice_event_builder
from sans_core.state.StateObjects.StateMaskDetectors import get_mask_builder
from sans_core.state.AllStates import get_all_states_builder
from sans_core.state.StateObjects.StateWavelength import get_wavelength_builder, StateWavelength
from sans_core.state.StateObjects.StateSave import get_save_builder
from sans_core.state.StateObjects.StateNormalizeToMonitor import get_normalize_to_monitor_builder
from sans_core.state.StateObjects.StateScale import get_scale_builder
from sans_core.state.StateObjects.StateCalculateTransmission import get_calculate_transmission
from sans_core.state.StateObjects.StateWavelengthAndPixelAdjustment import get_wavelength_and_pixel_adjustment_builder
from sans_core.state.StateObjects.StateAdjustment import get_adjustment_builder
from sans_core.state.StateObjects.StateConvertToQ import get_convert_to_q_builder

from sans_core.common.enums import (
    SANSFacility,
    ReductionMode,
    ReductionDimensionality,
    FitModeForMerge,
    RebinType,
    RangeStepType,
    SaveType,
    FitType,
    SampleShape,
    SANSInstrument,
)
from sans_core.state.StateObjects.state_instrument_info import StateInstrumentInfo
from sans_core.test_helper.file_information_mock import SANSFileInformationMock


class TestDirector(object):
    """The purpose of this builder is to create a valid state object for tests"""

    def __init__(self):
        super(TestDirector, self).__init__()
        self.data_state = None
        self.move_state = None
        self.reduction_state = None
        self.slice_state = None
        self.mask_state = None
        self.wavelength_state = None
        self.save_state = None
        self.scale_state = None
        self.adjustment_state = None
        self.convert_to_q_state = None
        self.inst_info_state = None

    def set_states(
        self,
        data_state=None,
        move_state=None,
        reduction_state=None,
        slice_state=None,
        mask_state=None,
        wavelength_state=None,
        save_state=None,
        scale_state=None,
        adjustment_state=None,
        convert_to_q_state=None,
    ):
        self.data_state = data_state
        self.data_state = data_state
        self.move_state = move_state
        self.reduction_state = reduction_state
        self.slice_state = slice_state
        self.mask_state = mask_state
        self.wavelength_state = wavelength_state
        self.save_state = save_state
        self.scale_state = scale_state
        self.adjustment_state = adjustment_state
        self.convert_to_q_state = convert_to_q_state

    def construct(self):
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(run_number=22024, instrument=SANSInstrument.SANS2D)

        # Build the SANSStateData
        if self.data_state is None:
            data_builder = get_data_builder(facility, file_information)
            data_builder.set_sample_scatter("SANS2D00022024")
            data_builder.set_can_scatter("SANS2D00022024")
            self.data_state = data_builder.build()

        # Build the SANSStateMove
        if self.move_state is None:
            move_builder = get_move_builder(self.data_state)
            if hasattr(move_builder, "set_HAB_x_translation_correction"):
                move_builder.set_HAB_x_translation_correction(21.2)
            move_builder.set_LAB_x_translation_correction(12.1)
            self.move_state = move_builder.build()

        if self.inst_info_state is None:
            self.inst_info_state = StateInstrumentInfo.build_from_data_info(self.data_state)

        # Build the SANSStateReduction
        if self.reduction_state is None:
            state_reduction = StateReductionMode()
            state_reduction.reduction_dimensionality = ReductionDimensionality.ONE_DIM
            state_reduction.merge_fit_mode = FitModeForMerge.BOTH
            state_reduction.merge_shift = 324.2
            state_reduction.merge_scale = 3420.98
            state_reduction.reduction_mode = ReductionMode.MERGED
            self.reduction_state = state_reduction

        # Build the SANSStateSliceEvent
        if self.slice_state is None:
            slice_builder = get_slice_event_builder(self.data_state)
            slice_builder.set_start_time([0.1, 1.3])
            slice_builder.set_end_time([0.2, 1.6])
            self.slice_state = slice_builder.build()

        # Build the SANSStateMask
        if self.mask_state is None:
            mask_builder = get_mask_builder(self.data_state)
            mask_builder.set_radius_min(10.0)
            mask_builder.set_radius_max(20.0)
            self.mask_state = mask_builder.build()

        # Build the SANSStateWavelength
        if self.wavelength_state is None:
            wavelength_builder = get_wavelength_builder(self.data_state)
            wavelength_range = (1.0, 10.0)
            wavelength_builder.state.wavelength_interval.wavelength_full_range = wavelength_range
            wavelength_builder.state.wavelength_interval.selected_ranges = [wavelength_range]
            wavelength_builder.set_wavelength_step_type(RangeStepType.LIN)
            self.wavelength_state = wavelength_builder.build()

        # Build the SANSStateSave
        if self.save_state is None:
            save_builder = get_save_builder(self.data_state)
            save_builder.set_user_specified_output_name("test_file_name")
            save_builder.set_file_format([SaveType.NEXUS])
            self.save_state = save_builder.build()

        # Build the SANSStateScale
        if self.scale_state is None:
            scale_builder = get_scale_builder(self.data_state, file_information)
            scale_builder.set_shape(SampleShape.FLAT_PLATE)
            scale_builder.set_width(1.0)
            scale_builder.set_height(2.0)
            scale_builder.set_thickness(3.0)
            scale_builder.set_scale(4.0)
            self.scale_state = scale_builder.build()

        instrument = self.data_state.instrument

        # Build the SANSAdjustmentState
        if self.adjustment_state is None:
            # NormalizeToMonitor
            normalize_to_monitor_builder = get_normalize_to_monitor_builder(self.data_state)
            normalize_to_monitor_builder.set_background_TOF_general_start(1000.0)
            normalize_to_monitor_builder.set_background_TOF_general_stop(2000.0)
            normalize_to_monitor_builder.set_incident_monitor(1)
            normalize_to_monitor_builder.set_rebin_type(RebinType.REBIN)
            normalize_to_monitor = normalize_to_monitor_builder.build()

            # CalculateTransmission
            calculate_transmission_obj = get_calculate_transmission(instrument=instrument)
            calculate_transmission_obj.transmission_monitor = 3
            calculate_transmission_obj.incident_monitor = 2
            wavelength_range = (1.0, 10.0)
            calculate_transmission_obj.wavelength_interval.wavelength_full_range = wavelength_range
            calculate_transmission_obj.wavelength_interval.selected_ranges = [wavelength_range]
            calculate_transmission_obj.wavelength_interval.wavelength_step = 2.0
            calculate_transmission_obj.wavelength_step_type = RangeStepType.LIN
            calculate_transmission_obj.rebin_type = RebinType.REBIN
            calculate_transmission_obj.background_TOF_general_start = 1000.0
            calculate_transmission_obj.background_TOF_general_stop = 2000.0

            calculate_transmission_obj.set_sample_fit_type(FitType.LINEAR)
            calculate_transmission_obj.set_sample_polynomial_order(0)
            calculate_transmission_obj.set_sample_wavelength_low(1.0)
            calculate_transmission_obj.set_sample_wavelength_high(10.0)
            calculate_transmission_obj.set_can_fit_type(FitType.POLYNOMIAL)
            calculate_transmission_obj.set_can_polynomial_order(3)
            calculate_transmission_obj.set_can_wavelength_low(10.0)
            calculate_transmission_obj.set_can_wavelength_high(20.0)

            # Wavelength and pixel adjustment
            wavelength_and_pixel_builder = get_wavelength_and_pixel_adjustment_builder(self.data_state)
            wavelength_range = (1.0, 10.0)
            wavelength_and_pixel_builder.state.wavelength_interval.wavelength_full_range = wavelength_range
            wavelength_and_pixel_builder.state.wavelength_interval.selected_ranges = [wavelength_range]
            wavelength_and_pixel_builder.state.wavelength_interval.wavelength_step = 2.0
            wavelength_and_pixel_builder.set_wavelength_step_type(RangeStepType.LIN)
            wavelength_and_pixel = wavelength_and_pixel_builder.build()

            # Adjustment
            adjustment_builder = get_adjustment_builder(self.data_state)
            adjustment_builder.set_normalize_to_monitor(normalize_to_monitor)
            adjustment_builder.set_calculate_transmission(calculate_transmission_obj)
            adjustment_builder.set_wavelength_and_pixel_adjustment(wavelength_and_pixel)
            self.adjustment_state = adjustment_builder.build()

        # SANSStateConvertToQ
        if self.convert_to_q_state is None:
            convert_to_q_builder = get_convert_to_q_builder(self.data_state)
            convert_to_q_builder.set_reduction_dimensionality(ReductionDimensionality.ONE_DIM)
            convert_to_q_builder.set_use_gravity(False)
            convert_to_q_builder.set_radius_cutoff(0.002)
            convert_to_q_builder.set_wavelength_cutoff(12.0)
            convert_to_q_builder.set_q_min(0.1)
            convert_to_q_builder.set_q_max(0.8)
            convert_to_q_builder.set_q_1d_rebin_string("0.1,0.01,0.8")
            convert_to_q_builder.set_use_q_resolution(False)
            self.convert_to_q_state = convert_to_q_builder.build()

        # Set the sub states on the SANSState
        state_builder = get_all_states_builder(self.data_state)
        state_builder.set_data(self.data_state)
        state_builder.set_move(self.move_state)
        state_builder.set_reduction(self.reduction_state)
        state_builder.set_slice(self.slice_state)
        state_builder.set_mask(self.mask_state)
        state_builder.set_wavelength(self.wavelength_state)
        state_builder.set_save(self.save_state)
        state_builder.set_scale(self.scale_state)
        state_builder.set_adjustment(self.adjustment_state)
        state_builder.set_convert_to_q(self.convert_to_q_state)
        state_builder.state.instrument_info = self.inst_info_state
        state = state_builder.build()
        state.wavelength = self._create_state_wavelength()
        return state

    @staticmethod
    def _create_state_wavelength() -> StateWavelength:
        obj = StateWavelength()
        wavelength_range = (1.0, 10.0)
        obj.wavelength_interval.wavelength_full_range = wavelength_range
        obj.wavelength_interval.selected_ranges = [wavelength_range]
        obj.wavelength_interval.wavelength_step = 2.0
        obj.wavelength_step_type = RangeStepType.LIN
        obj.rebin_type = RebinType.REBIN
        return obj
