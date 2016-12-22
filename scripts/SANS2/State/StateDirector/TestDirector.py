from SANS2.State.StateBuilder.SANSStateBuilder import get_state_builder
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateMoveBuilder import get_move_builder
from SANS2.State.StateBuilder.SANSStateReductionBuilder import get_reduction_builder
from SANS2.State.StateBuilder.SANSStateSliceEventBuilder import get_slice_event_builder
from SANS2.State.StateBuilder.SANSStateMaskBuilder import get_mask_builder
from SANS2.State.StateBuilder.SANSStateWavelengthBuilder import get_wavelength_builder
from SANS2.State.StateBuilder.SANSStateSaveBuilder import get_save_builder


from SANS2.Common.SANSEnumerations import (SANSFacility, ISISReductionMode, ReductionDimensionality,
                                           FitModeForMerge, RebinType, RangeStepType, SaveType)


class TestDirector(object):
    """ The purpose of this builder is to create a valid state object for tests"""
    def __init__(self):
        super(TestDirector, self).__init__()
        self.data_state = None
        self.move_state = None
        self.reduction_state = None
        self.slice_state = None
        self.mask_state = None
        self.wavelength_state = None
        self.save_state = None

    def set_states(self, data_state=None, move_state=None, reduction_state=None, slice_state=None,
                   mask_state=None, wavelength_state=None, save_state=None):
        self.data_state = data_state
        self.move_state = move_state
        self.reduction_state = reduction_state
        self.slice_state = slice_state
        self.mask_state = mask_state
        self.wavelength_state = wavelength_state
        self.save_state = save_state

    def construct(self):
        facility = SANSFacility.ISIS

        # Build the SANSStateData
        if self.data_state is None:
            data_builder = get_data_builder(facility)
            data_builder.set_sample_scatter("SANS2D00022024")
            data_builder.set_can_scatter("SANS2D00022024")
            self.data_state = data_builder.build()

        # Build the SANSStateMove
        if self.move_state is None:
            move_builder = get_move_builder(self.data_state)
            move_builder.set_HAB_x_translation_correction(21.2)
            move_builder.set_LAB_x_translation_correction(12.1)
            self.move_state = move_builder.build()

        # Build the SANSStateReduction
        if self.reduction_state is None:
            reduction_builder = get_reduction_builder(self.data_state)
            reduction_builder.set_reduction_mode(ISISReductionMode.Merged)
            reduction_builder.set_dimensionality(ReductionDimensionality.OneDim)
            reduction_builder.set_merge_fit_mode(FitModeForMerge.Both)
            reduction_builder.set_merge_shift(324.2)
            reduction_builder.set_merge_scale(3420.98)
            self.reduction_state = reduction_builder.build()

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
            wavelength_builder.set_wavelength_low(10.0)
            wavelength_builder.set_wavelength_high(20.0)
            wavelength_builder.set_wavelength_step(2.0)
            wavelength_builder.set_wavelength_step_type(RangeStepType.Lin)
            wavelength_builder.set_rebin_type(RebinType.Rebin)
            self.wavelength_state = wavelength_builder.build()

        # Build the SANSStateSave
        if self.save_state is None:
            save_builder = get_save_builder(self.data_state)
            save_builder.set_file_name("test_file_name")
            save_builder.set_file_format([SaveType.Nexus])
            self.save_state = save_builder.build()

        # Set the sub states on the SANSState
        state_builder = get_state_builder(self.data_state)
        state_builder.set_data(self.data_state)
        state_builder.set_move(self.move_state)
        state_builder.set_reduction(self.reduction_state)
        state_builder.set_slice(self.slice_state)
        state_builder.set_mask(self.mask_state)
        state_builder.set_wavelength(self.wavelength_state)
        state_builder.set_save(self.save_state)
        return state_builder.build()
