# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abc

from sans.common.enums import DetectorType, RangeStepType, RebinType, FitType, DataType, FitModeForMerge
from sans.common.general_functions import get_ranges_from_event_slice_setting, get_ranges_for_rebin_setting, get_ranges_for_rebin_array
from sans.state.AllStates import AllStates
from sans.state.IStateParser import IStateParser
from sans.state.StateObjects.StateAdjustment import StateAdjustment
from sans.state.StateObjects.StateCalculateTransmission import get_calculate_transmission
from sans.state.StateObjects.StateCompatibility import StateCompatibility
from sans.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans.state.StateObjects.StateMaskDetectors import get_mask_builder
from sans.state.StateObjects.StateMoveDetectors import get_move_builder
from sans.state.StateObjects.StateNormalizeToMonitor import get_normalize_to_monitor_builder
from sans.state.StateObjects.StatePolarization import StatePolarization
from sans.state.StateObjects.StateReductionMode import StateReductionMode
from sans.state.StateObjects.StateSave import StateSave
from sans.state.StateObjects.StateScale import StateScale
from sans.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans.state.StateObjects.StateWavelength import StateWavelength
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment
from sans.user_file.parser_helpers.wavelength_parser import parse_range_wavelength
from sans.user_file.settings_tags import (
    TubeCalibrationFileId,
    MaskId,
    LimitsId,
    complex_range,
    GravityId,
    QResolutionId,
    OtherId,
    MonId,
    TransId,
    FitId,
    BackId,
    SampleId,
    SetId,
    DetectorId,
    simple_range,
    rebin_string_values,
)


class ParsedDictConverter(IStateParser):
    def __init__(self, file_information, existing_all_states: AllStates = None):
        super(ParsedDictConverter, self).__init__()

        self._cached_result = None
        data = self.get_state_data(file_information)
        self._instrument = data.instrument
        self._all_states = existing_all_states

    @property
    def _input_dict(self):
        if not self._cached_result:
            self._cached_result = self._get_input_dict()
            # Ensure we always have a dict
            self._cached_result = self._cached_result if self._cached_result else {}
        return self._cached_result

    @abc.abstractmethod
    def _get_input_dict(self):
        """
        Gets the dictionary to translate as an input dictionary from the inheriting adapter
        :return: Dictionary to translate
        """
        pass

    def get_state_data(self, file_information):
        data = super().get_state_data(file_information)
        if DetectorId.INSTRUMENT in self._input_dict:
            data.instrument = self._input_dict[DetectorId.INSTRUMENT][0]
        return data

    def get_state_adjustment(self, file_information):
        state = self._all_states.adjustment if self._all_states else StateAdjustment()
        # Get the wide angle correction setting
        self._set_single_entry(state, "wide_angle_correction", SampleId.PATH)

        state.calculate_transmission = self.get_state_calculate_transmission()
        state.normalize_to_monitor = self.get_state_normalize_to_monitor(file_information=file_information)
        state.wavelength_and_pixel_adjustment = self.get_state_wavelength_and_pixel_adjustment()

        if TubeCalibrationFileId.FILE in self._input_dict:
            state.calibration = _get_last_element(self._input_dict.get(TubeCalibrationFileId.FILE))

        state.wavelength_and_pixel_adjustment.idf_path = self.get_state_data(file_information).idf_file_path

        return state

    def get_state_calculate_transmission(self):
        state = (
            self._all_states.adjustment.calculate_transmission
            if self._all_states
            else get_calculate_transmission(instrument=self._instrument)
        )

        self._set_single_entry(state, "transmission_radius_on_detector", TransId.RADIUS, apply_to_value=_convert_mm_to_m)

        # List of transmission roi files
        if TransId.ROI in self._input_dict:
            trans_roi = self._input_dict[TransId.ROI]
            state.transmission_roi_files = trans_roi

        # List of transmission mask files
        if TransId.MASK in self._input_dict:
            trans_mask = self._input_dict[TransId.MASK]
            state.transmission_mask_files = trans_mask

        # The prompt peak correction values
        _set_prompt_peak_correction(state, self._input_dict)

        # The transmission spectrum
        if TransId.SPEC in self._input_dict:
            trans_spec = self._input_dict[TransId.SPEC]
            trans_spec = trans_spec[-1]
            state.transmission_monitor = trans_spec

        # The incident monitor spectrum for transmission calculation
        if MonId.SPECTRUM in self._input_dict:
            mon_spectrum = self._input_dict[MonId.SPECTRUM]
            mon_spectrum = [spec for spec in mon_spectrum if spec.is_trans]
            for spec in mon_spectrum:
                rebin_type = RebinType.INTERPOLATING_REBIN if spec.interpolate else RebinType.REBIN
                state.rebin_type = rebin_type

                # We have to check if the spectrum is None, this can be the case when the user wants to use the
                # default incident monitor spectrum
                if spec.spectrum:
                    state.incident_monitor = spec.spectrum

        # The general background settings
        _set_background_tof_general(state, self._input_dict)

        # The monitor-specific background settings
        _set_background_tof_monitor(state, self._input_dict)

        # The roi-specific background settings
        if BackId.TRANS in self._input_dict:
            back_trans = self._input_dict[BackId.TRANS]
            back_trans = back_trans[-1]
            state.background_TOF_roi_start = back_trans.start
            state.background_TOF_roi_stop = back_trans.stop

        # Set the fit settings
        if FitId.GENERAL in self._input_dict:
            fit_general = self._input_dict[FitId.GENERAL]
            # We can have settings for both the sample or the can or individually
            # There can be three types of settings:
            # 1. Clearing the fit setting
            # 2. General settings where the entry data_type is not specified. Settings apply to both sample and can
            # 3. Sample settings
            # 4. Can settings
            # We first apply the general settings. Specialized settings for can or sample override the general settings
            # As usual if there are multiple settings for a specific case, then the last in the list is used.
            can = state.fit[DataType.CAN.value]
            sample = state.fit[DataType.SAMPLE.value]

            # 1 Fit type settings
            clear_settings = [item for item in fit_general if item.data_type is None and item.fit_type is FitType.NO_FIT]

            if clear_settings:
                clear_settings = clear_settings[-1]
                # Will set the fitting to NoFit
                sample.fit_type = clear_settings.fit_type
                can.fit_type = clear_settings.fit_type

            # 2. General settings
            general_settings = [item for item in fit_general if item.data_type is None and item.fit_type is not FitType.NO_FIT]
            if general_settings:
                general_settings = general_settings[-1]

                sample.fit_type = general_settings.fit_type
                sample.polynomial_order = general_settings.polynomial_order
                sample.wavelength_low = general_settings.start
                sample.wavelength_high = general_settings.stop

                can.fit_type = general_settings.fit_type
                can.polynomial_order = general_settings.polynomial_order
                can.wavelength_low = general_settings.start
                can.wavelength_high = general_settings.stop

            # 3. Sample settings
            sample_settings = [item for item in fit_general if item.data_type is DataType.SAMPLE]
            if sample_settings:
                sample_settings = sample_settings[-1]
                sample.fit_type = sample_settings.fit_type
                sample.polynomial_order = sample_settings.polynomial_order
                sample.wavelength_low = sample_settings.start
                sample.wavelength_high = sample_settings.stop

            # 4. Can settings
            can_settings = [item for item in fit_general if item.data_type is DataType.CAN]
            if can_settings:
                can_settings = can_settings[-1]
                can.fit_type = can_settings.fit_type
                can.polynomial_order = can_settings.polynomial_order
                can.wavelength_low = can_settings.start
                can.wavelength_high = can_settings.stop

        # Set the wavelength default configuration
        _set_wavelength_limits(state, self._input_dict)

        # Set the full wavelength range. Note that this can currently only be set from the ISISCommandInterface
        if OtherId.USE_FULL_WAVELENGTH_RANGE in self._input_dict:
            use_full_wavelength_range = self._input_dict[OtherId.USE_FULL_WAVELENGTH_RANGE]
            use_full_wavelength_range = use_full_wavelength_range[-1]
            state.use_full_wavelength_range = use_full_wavelength_range
        return state

    def get_state_compatibility(self):
        state = self._all_states.compatibility if self._all_states else StateCompatibility()
        if LimitsId.EVENTS_BINNING in self._input_dict:
            events_binning = self._input_dict[LimitsId.EVENTS_BINNING]
            events_binning = events_binning[-1]
            state.time_rebin_string = events_binning

        if OtherId.USE_COMPATIBILITY_MODE in self._input_dict:
            use_compatibility_mode = self._input_dict[OtherId.USE_COMPATIBILITY_MODE]
            use_compatibility_mode = use_compatibility_mode[-1]
            state.use_compatibility_mode = use_compatibility_mode

        if OtherId.USE_EVENT_SLICE_OPTIMISATION in self._input_dict:
            use_event_slice_optimisation = self._input_dict[OtherId.USE_EVENT_SLICE_OPTIMISATION]
            use_event_slice_optimisation = use_event_slice_optimisation[-1]
            state.use_event_slice_optimisation = use_event_slice_optimisation

        return state

    def get_state_convert_to_q(self):
        state = self._all_states.convert_to_q if self._all_states else StateConvertToQ()
        # Get the radius cut off if any is present
        self._set_single_entry(state, "radius_cutoff", LimitsId.RADIUS_CUT, apply_to_value=_convert_mm_to_m)

        # Get the wavelength cut off if any is present
        self._set_single_entry(state, "wavelength_cutoff", LimitsId.WAVELENGTH_CUT)

        # Get the 1D q values
        if LimitsId.Q in self._input_dict:
            limits_q = self._input_dict[LimitsId.Q]
            limits_q = limits_q[-1]
            state.q_min = limits_q.min
            state.q_max = limits_q.max
            state.q_1d_rebin_string = limits_q.rebin_string

        # Get the 2D q values
        if LimitsId.QXY in self._input_dict:
            limits_qxy = self._input_dict[LimitsId.QXY]
            limits_qxy = limits_qxy[-1]
            # Now we have to check if we have a simple pattern or a more complex pattern at hand
            is_complex = isinstance(limits_qxy, complex_range)
            state.q_xy_max = limits_qxy.stop
            if is_complex:
                # Note that it has not been implemented in the old reducer, but the documentation is
                #  suggesting that it is available. Hence we throw here.
                raise RuntimeError("Qxy cannot handle settings of type: L/Q l1,dl1,l3,dl2,l2 [/LIN|/LOG] ")
            else:
                state.q_xy_step = limits_qxy.step

        # Get the Gravity settings
        self._set_single_entry(state, "use_gravity", GravityId.ON_OFF)
        self._set_single_entry(state, "gravity_extra_length", GravityId.EXTRA_LENGTH)

        # Get the QResolution settings set_q_resolution_delta_r
        self._set_single_entry(state, "use_q_resolution", QResolutionId.ON)
        self._set_single_entry(state, "q_resolution_delta_r", QResolutionId.DELTA_R, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "q_resolution_collimation_length", QResolutionId.COLLIMATION_LENGTH)
        self._set_single_entry(state, "q_resolution_a1", QResolutionId.A1, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "q_resolution_a2", QResolutionId.A2, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "moderator_file", QResolutionId.MODERATOR)
        self._set_single_entry(state, "q_resolution_h1", QResolutionId.H1, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "q_resolution_h2", QResolutionId.H2, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "q_resolution_w1", QResolutionId.W1, apply_to_value=_convert_mm_to_m)
        self._set_single_entry(state, "q_resolution_w2", QResolutionId.W2, apply_to_value=_convert_mm_to_m)

        # ------------------------
        # Reduction Dimensionality
        # ------------------------
        self._set_single_entry(state, "reduction_dimensionality", OtherId.REDUCTION_DIMENSIONALITY)
        return state

    # We have taken the implementation originally provided, so we can't help the complexity
    def get_state_mask(self, file_information):  # noqa: C901
        state_builder = get_mask_builder(data_info=self.get_state_data(file_information=file_information))

        # We have to inject an existing state object here, this is wrong but legacy code *shrug*
        if self._all_states:
            state_builder.state = self._all_states.mask

        if MaskId.LINE in self._input_dict:
            mask_lines = self._input_dict[MaskId.LINE]
            mask_line = mask_lines[-1]
            # We need the width and the angle
            angle = mask_line.angle
            width = _convert_mm_to_m(mask_line.width)
            # The position is already specified in meters in the user file
            pos1 = mask_line.x
            pos2 = mask_line.y
            if angle is None or width is None:
                raise RuntimeError(
                    "UserFileStateDirector: You specified a line mask without an angle or a width."
                    "The parameters were: width {0}; angle {1}; x {2}; y {3}".format(width, angle, pos1, pos2)
                )
            pos1 = 0.0 if pos1 is None else pos1
            pos2 = 0.0 if pos2 is None else pos2

            state_builder.set_beam_stop_arm_width(width)
            state_builder.set_beam_stop_arm_angle(angle)
            state_builder.set_beam_stop_arm_pos1(pos1)
            state_builder.set_beam_stop_arm_pos2(pos2)

        # ---------------------------------
        # 2. General time mask
        # ---------------------------------
        if MaskId.TIME in self._input_dict:
            mask_time_general = self._input_dict[MaskId.TIME]
            start_time = []
            stop_time = []
            for times in mask_time_general:
                if times.start > times.start:
                    raise RuntimeError(
                        "UserFileStateDirector: You specified a general time mask with a start time {0}"
                        " which is larger than the stop time {1} of the mask. This is not"
                        " valid.".format(times.start, times.stop)
                    )
                start_time.append(times.start)
                stop_time.append(times.stop)
            state_builder.set_bin_mask_general_start(start_time)
            state_builder.set_bin_mask_general_stop(stop_time)

        # ---------------------------------
        # 3. Detector-bound time mask
        # ---------------------------------
        if MaskId.TIME_DETECTOR in self._input_dict:
            mask_times = self._input_dict[MaskId.TIME_DETECTOR]
            start_times_hab = []
            stop_times_hab = []
            start_times_lab = []
            stop_times_lab = []
            for times in mask_times:
                if times.start > times.start:
                    raise RuntimeError(
                        "UserFileStateDirector: You specified a general time mask with a start time {0}"
                        " which is larger than the stop time {1} of the mask. This is not"
                        " valid.".format(times.start, times.stop)
                    )
                if times.detector_type is DetectorType.HAB:
                    start_times_hab.append(times.start)
                    stop_times_hab.append(times.stop)
                elif times.detector_type is DetectorType.LAB:
                    start_times_lab.append(times.start)
                    stop_times_lab.append(times.stop)
                else:
                    RuntimeError("UserFileStateDirector: The specified detector {0} is not known".format(times.detector_type))
            if start_times_hab:
                state_builder.set_HAB_bin_mask_start(start_times_hab)
            if stop_times_hab:
                state_builder.set_HAB_bin_mask_stop(stop_times_hab)
            if start_times_lab:
                state_builder.set_LAB_bin_mask_start(start_times_lab)
            if stop_times_lab:
                state_builder.set_LAB_bin_mask_stop(stop_times_lab)

        # ---------------------------------
        # 4. Clear detector
        # ---------------------------------
        if MaskId.CLEAR_DETECTOR_MASK in self._input_dict:
            clear_detector_mask = self._input_dict[MaskId.CLEAR_DETECTOR_MASK]
            # We select the entry which was added last.
            clear_detector_mask = clear_detector_mask[-1]
            state_builder.set_clear(clear_detector_mask)

        # ---------------------------------
        # 5. Clear time
        # ---------------------------------
        if MaskId.CLEAR_TIME_MASK in self._input_dict:
            clear_time_mask = self._input_dict[MaskId.CLEAR_TIME_MASK]
            # We select the entry which was added last.
            clear_time_mask = clear_time_mask[-1]
            state_builder.set_clear_time(clear_time_mask)

        # ---------------------------------
        # 6. Single Spectrum
        # ---------------------------------
        if MaskId.SINGLE_SPECTRUM_MASK in self._input_dict:
            single_spectra = self._input_dict[MaskId.SINGLE_SPECTRUM_MASK]
            # Note that we are using an unusual setter here. Check mask.py for why we are doing this.
            state_builder.set_single_spectra_on_detector(single_spectra)

        # ---------------------------------
        # 7. Spectrum Range
        # ---------------------------------
        if MaskId.SPECTRUM_RANGE_MASK in self._input_dict:
            spectrum_ranges = self._input_dict[MaskId.SPECTRUM_RANGE_MASK]
            start_range = []
            stop_range = []
            for spectrum_range in spectrum_ranges:
                if spectrum_range.start > spectrum_range.start:
                    raise RuntimeError(
                        "UserFileStateDirector: You specified a spectrum range with a start value {0}"
                        " which is larger than the stop value {1}. This is not"
                        " valid.".format(spectrum_range.start, spectrum_range.stop)
                    )
                start_range.append(spectrum_range.start)
                stop_range.append(spectrum_range.stop)
            # Note that we are using an unusual setter here. Check mask.py for why we are doing this.
            state_builder.set_spectrum_range_on_detector(start_range, stop_range)

        # ---------------------------------
        # 8. Vertical single strip
        # ---------------------------------
        if MaskId.VERTICAL_SINGLE_STRIP_MASK in self._input_dict:
            single_vertical_strip_masks = self._input_dict[MaskId.VERTICAL_SINGLE_STRIP_MASK]
            entry_hab = []
            entry_lab = []
            for single_vertical_strip_mask in single_vertical_strip_masks:
                if single_vertical_strip_mask.detector_type is DetectorType.HAB:
                    entry_hab.append(single_vertical_strip_mask.entry)
                elif single_vertical_strip_mask.detector_type is DetectorType.LAB:
                    entry_lab.append(single_vertical_strip_mask.entry)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The vertical single strip mask {0} has an unknown detector {1} associated with it.".format(
                            single_vertical_strip_mask.entry, single_vertical_strip_mask.detector_type
                        )
                    )
            if entry_hab:
                state_builder.set_HAB_single_vertical_strip_mask(entry_hab)
            if entry_lab:
                state_builder.set_LAB_single_vertical_strip_mask(entry_lab)

        # ---------------------------------
        # 9. Vertical range strip
        # ---------------------------------
        if MaskId.VERTICAL_RANGE_STRIP_MASK in self._input_dict:
            range_vertical_strip_masks = self._input_dict[MaskId.VERTICAL_RANGE_STRIP_MASK]
            start_hab = []
            stop_hab = []
            start_lab = []
            stop_lab = []
            for range_vertical_strip_mask in range_vertical_strip_masks:
                if range_vertical_strip_mask.detector_type is DetectorType.HAB:
                    start_hab.append(range_vertical_strip_mask.start)
                    stop_hab.append(range_vertical_strip_mask.stop)
                elif range_vertical_strip_mask.detector_type is DetectorType.LAB:
                    start_lab.append(range_vertical_strip_mask.start)
                    stop_lab.append(range_vertical_strip_mask.stop)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The vertical range strip mask {0} has an unknown detector {1} associated with it.".format(
                            range_vertical_strip_mask.entry, range_vertical_strip_mask.detector_type
                        )
                    )
            if start_hab:
                state_builder.set_HAB_range_vertical_strip_start(start_hab)
            if stop_hab:
                state_builder.set_HAB_range_vertical_strip_stop(stop_hab)
            if start_lab:
                state_builder.set_LAB_range_vertical_strip_start(start_lab)
            if stop_lab:
                state_builder.set_LAB_range_vertical_strip_stop(stop_lab)

        # ---------------------------------
        # 10. Horizontal single strip
        # ---------------------------------
        if MaskId.HORIZONTAL_SINGLE_STRIP_MASK in self._input_dict:
            single_horizontal_strip_masks = self._input_dict[MaskId.HORIZONTAL_SINGLE_STRIP_MASK]
            entry_hab = []
            entry_lab = []
            for single_horizontal_strip_mask in single_horizontal_strip_masks:
                if single_horizontal_strip_mask.detector_type is DetectorType.HAB:
                    entry_hab.append(single_horizontal_strip_mask.entry)
                elif single_horizontal_strip_mask.detector_type is DetectorType.LAB:
                    entry_lab.append(single_horizontal_strip_mask.entry)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The horizontal single strip mask {0} has an unknown "
                        "detector {1} associated"
                        " with it.".format(single_horizontal_strip_mask.entry, single_horizontal_strip_mask.detector_type)
                    )
            if entry_hab:
                state_builder.set_HAB_single_horizontal_strip_mask(entry_hab)
            if entry_lab:
                state_builder.set_LAB_single_horizontal_strip_mask(entry_lab)

        # ---------------------------------
        # 11. Horizontal range strip
        # ---------------------------------
        if MaskId.HORIZONTAL_RANGE_STRIP_MASK in self._input_dict:
            range_horizontal_strip_masks = self._input_dict[MaskId.HORIZONTAL_RANGE_STRIP_MASK]
            start_hab = []
            stop_hab = []
            start_lab = []
            stop_lab = []
            for range_horizontal_strip_mask in range_horizontal_strip_masks:
                if range_horizontal_strip_mask.detector_type is DetectorType.HAB:
                    start_hab.append(range_horizontal_strip_mask.start)
                    stop_hab.append(range_horizontal_strip_mask.stop)
                elif range_horizontal_strip_mask.detector_type is DetectorType.LAB:
                    start_lab.append(range_horizontal_strip_mask.start)
                    stop_lab.append(range_horizontal_strip_mask.stop)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The vertical range strip mask {0} has an unknown detector {1} associated with it.".format(
                            range_horizontal_strip_mask.entry, range_horizontal_strip_mask.detector_type
                        )
                    )
            if start_hab:
                state_builder.set_HAB_range_horizontal_strip_start(start_hab)
            if stop_hab:
                state_builder.set_HAB_range_horizontal_strip_stop(stop_hab)
            if start_lab:
                state_builder.set_LAB_range_horizontal_strip_start(start_lab)
            if stop_lab:
                state_builder.set_LAB_range_horizontal_strip_stop(stop_lab)

        # ---------------------------------
        # 12. Block
        # ---------------------------------
        if MaskId.BLOCK in self._input_dict:
            blocks = self._input_dict[MaskId.BLOCK]
            horizontal_start_hab = []
            horizontal_stop_hab = []
            vertical_start_hab = []
            vertical_stop_hab = []
            horizontal_start_lab = []
            horizontal_stop_lab = []
            vertical_start_lab = []
            vertical_stop_lab = []

            for block in blocks:
                if block.horizontal1 > block.horizontal2 or block.vertical1 > block.vertical2:
                    raise RuntimeError(
                        "UserFileStateDirector: The block mask seems to have inconsistent entries. "
                        "The values are horizontal_start {0}; horizontal_stop {1}; vertical_start {2};"
                        " vertical_stop {3}".format(block.horizontal1, block.horizontal2, block.vertical1, block.vertical2)
                    )
                if block.detector_type is DetectorType.HAB:
                    horizontal_start_hab.append(block.horizontal1)
                    horizontal_stop_hab.append(block.horizontal2)
                    vertical_start_hab.append(block.vertical1)
                    vertical_stop_hab.append(block.vertical2)
                elif block.detector_type is DetectorType.LAB:
                    horizontal_start_lab.append(block.horizontal1)
                    horizontal_stop_lab.append(block.horizontal2)
                    vertical_start_lab.append(block.vertical1)
                    vertical_stop_lab.append(block.vertical2)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The block mask has an unknown detector {0} associated with it.".format(block.detector_type)
                    )
            if horizontal_start_hab:
                state_builder.set_HAB_block_horizontal_start(horizontal_start_hab)
            if horizontal_stop_hab:
                state_builder.set_HAB_block_horizontal_stop(horizontal_stop_hab)
            if vertical_start_lab:
                state_builder.set_LAB_block_vertical_start(vertical_start_lab)
            if vertical_stop_lab:
                state_builder.set_LAB_block_vertical_stop(vertical_stop_lab)

        # ---------------------------------
        # 13. Block cross
        # ---------------------------------
        if MaskId.BLOCK_CROSS in self._input_dict:
            block_crosses = self._input_dict[MaskId.BLOCK_CROSS]
            horizontal_hab = []
            vertical_hab = []
            horizontal_lab = []
            vertical_lab = []
            for block_cross in block_crosses:
                if block_cross.detector_type is DetectorType.HAB:
                    horizontal_hab.append(block_cross.horizontal)
                    vertical_hab.append(block_cross.vertical)
                elif block_cross.detector_type is DetectorType.LAB:
                    horizontal_lab.append(block_cross.horizontal)
                    vertical_lab.append(block_cross.vertical)
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: The block cross mask has an unknown detector {0} associated with it.".format(
                            block_cross.detector_type
                        )
                    )
            if horizontal_hab:
                state_builder.set_HAB_block_cross_horizontal(horizontal_hab)
            if vertical_hab:
                state_builder.set_HAB_block_cross_vertical(vertical_hab)
            if horizontal_lab:
                state_builder.set_LAB_block_cross_horizontal(horizontal_lab)
            if vertical_lab:
                state_builder.set_LAB_block_cross_vertical(vertical_lab)

        # ------------------------------------------------------------
        # 14. Angles --> they are specified in L/Phi
        # -----------------------------------------------------------
        if LimitsId.ANGLE in self._input_dict:
            angles = self._input_dict[LimitsId.ANGLE]
            # Should the user have chosen several values, then the last element is selected
            angle = angles[-1]
            state_builder.set_phi_min(angle.min)
            state_builder.set_phi_max(angle.max)
            state_builder.set_use_mask_phi_mirror(angle.use_mirror)

        # ------------------------------------------------------------
        # 15. Maskfiles
        # -----------------------------------------------------------
        if MaskId.FILE in self._input_dict:
            mask_files = self._input_dict[MaskId.FILE]
            state_builder.set_mask_files(mask_files)

        # ------------------------------------------------------------
        # 16. Radius masks
        # -----------------------------------------------------------
        if LimitsId.RADIUS in self._input_dict:
            radii = self._input_dict[LimitsId.RADIUS]
            # Should the user have chosen several values, then the last element is selected
            radius = radii[-1]
            if radius.start > radius.stop > 0:
                raise RuntimeError(
                    "UserFileStateDirector: The inner radius {0} appears to be larger that the outer radius {1} of the mask.".format(
                        radius.start, radius.stop
                    )
                )
            min_value = None if radius.start is None else _convert_mm_to_m(radius.start)
            max_value = None if radius.stop is None else _convert_mm_to_m(radius.stop)
            state_builder.set_radius_min(min_value)
            state_builder.set_radius_max(max_value)
        return state_builder.build()

    # We have taken the implementation originally provided, so we can't help the complexity
    def get_state_move(self, file_information):  # noqa : C901
        state_builder = get_move_builder(data_info=self.get_state_data(file_information=file_information))

        if self._all_states:
            state_builder.state = self._all_states.move

        if DetectorId.CORRECTION_X in self._input_dict:
            corrections_in_x = self._input_dict[DetectorId.CORRECTION_X]
            for correction_x in corrections_in_x:
                if correction_x.detector_type is DetectorType.HAB:
                    state_builder.set_HAB_x_translation_correction(_convert_mm_to_m(correction_x.entry))
                elif correction_x.detector_type is DetectorType.LAB:
                    state_builder.set_LAB_x_translation_correction(_convert_mm_to_m(correction_x.entry))
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: An unknown detector {0} was used for the x correction.".format(correction_x.detector_type)
                    )

        if DetectorId.CORRECTION_Y in self._input_dict:
            corrections_in_y = self._input_dict[DetectorId.CORRECTION_Y]
            for correction_y in corrections_in_y:
                if correction_y.detector_type is DetectorType.HAB:
                    state_builder.set_HAB_y_translation_correction(_convert_mm_to_m(correction_y.entry))
                elif correction_y.detector_type is DetectorType.LAB:
                    state_builder.set_LAB_y_translation_correction(_convert_mm_to_m(correction_y.entry))
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: An unknown detector {0} was used for the y correction.".format(correction_y.detector_type)
                    )

        if DetectorId.CORRECTION_Z in self._input_dict:
            corrections_in_z = self._input_dict[DetectorId.CORRECTION_Z]
            for correction_z in corrections_in_z:
                if correction_z.detector_type is DetectorType.HAB:
                    state_builder.set_HAB_z_translation_correction(_convert_mm_to_m(correction_z.entry))
                elif correction_z.detector_type is DetectorType.LAB:
                    state_builder.set_LAB_z_translation_correction(_convert_mm_to_m(correction_z.entry))
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: An unknown detector {0} was used for the z correction.".format(correction_z.detector_type)
                    )

        # ---------------------------
        # Correction for Rotation
        # ---------------------------
        if DetectorId.CORRECTION_ROTATION in self._input_dict:
            rotation_correction = self._input_dict[DetectorId.CORRECTION_ROTATION]
            rotation_correction = rotation_correction[-1]
            if rotation_correction.detector_type is DetectorType.HAB:
                state_builder.set_HAB_rotation_correction(rotation_correction.entry)
            elif rotation_correction.detector_type is DetectorType.LAB:
                state_builder.set_LAB_rotation_correction(rotation_correction.entry)
            else:
                raise RuntimeError(
                    "UserFileStateDirector: An unknown detector {0} was used for the rotation correction.".format(
                        rotation_correction.detector_type
                    )
                )

        # ---------------------------
        # Correction for Radius
        # ---------------------------
        if DetectorId.CORRECTION_RADIUS in self._input_dict:
            radius_corrections = self._input_dict[DetectorId.CORRECTION_RADIUS]
            for radius_correction in radius_corrections:
                if radius_correction.detector_type is DetectorType.HAB:
                    state_builder.set_HAB_radius_correction(_convert_mm_to_m(radius_correction.entry))
                elif radius_correction.detector_type is DetectorType.LAB:
                    state_builder.set_LAB_radius_correction(_convert_mm_to_m(radius_correction.entry))
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: An unknown detector {0} was used for the radius correction.".format(
                            radius_correction.detector_type
                        )
                    )

        # ---------------------------
        # Correction for Translation
        # ---------------------------
        if DetectorId.CORRECTION_TRANSLATION in self._input_dict:
            side_corrections = self._input_dict[DetectorId.CORRECTION_TRANSLATION]
            for side_correction in side_corrections:
                if side_correction.detector_type is DetectorType.HAB:
                    state_builder.set_HAB_side_correction(_convert_mm_to_m(side_correction.entry))
                elif side_correction.detector_type is DetectorType.LAB:
                    state_builder.set_LAB_side_correction(_convert_mm_to_m(side_correction.entry))
                else:
                    raise RuntimeError(
                        "UserFileStateDirector: An unknown detector {0} was used for the side correction.".format(
                            side_correction.detector_type
                        )
                    )

        # ---------------------------
        # Tilt
        # ---------------------------
        if DetectorId.CORRECTION_X_TILT in self._input_dict:
            tilt_correction = self._input_dict[DetectorId.CORRECTION_X_TILT]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.HAB:
                state_builder.set_HAB_x_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.LAB:
                state_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError(
                    "UserFileStateDirector: An unknown detector {0} was used for the tilt correction.".format(tilt_correction.detector_type)
                )

        if DetectorId.CORRECTION_Y_TILT in self._input_dict:
            tilt_correction = self._input_dict[DetectorId.CORRECTION_Y_TILT]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.HAB:
                state_builder.set_HAB_y_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.LAB:
                state_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError(
                    "UserFileStateDirector: An unknown detector {0} was used for the tilt correction.".format(tilt_correction.detector_type)
                )

        # ---------------------------
        # Sample offset
        # ---------------------------
        self._set_single_entry(state_builder.state, "sample_offset", SampleId.OFFSET, apply_to_value=_convert_mm_to_m)

        # ---------------------------
        # Monitor offsets
        # ---------------------------

        def parse_shift(key_to_parse, spec_num):
            monitor_n_shift = self._input_dict[key_to_parse]
            # Should the user have chosen several values, then the last element is selected
            monitor_n_shift = monitor_n_shift[-1]

            if spec_num == 4:  # All detectors have "M4"
                state_builder.state.monitor_4_offset = _convert_mm_to_m(monitor_n_shift)
            elif spec_num == 5 and hasattr(state_builder.state, "monitor_5_offset"):
                state_builder.state.monitor_5_offset = _convert_mm_to_m(monitor_n_shift)

        if TransId.SPEC_4_SHIFT in self._input_dict:
            parse_shift(key_to_parse=TransId.SPEC_4_SHIFT, spec_num=4)

        if TransId.SPEC_5_SHIFT in self._input_dict:
            parse_shift(key_to_parse=TransId.SPEC_5_SHIFT, spec_num=5)

        # ---------------------------
        # Beam Centre, this can be for HAB and LAB
        # ---------------------------
        if SetId.CENTRE in self._input_dict:
            beam_centres = self._input_dict[SetId.CENTRE]
            beam_centres_for_lab = [beam_centre for beam_centre in beam_centres if beam_centre.detector_type is DetectorType.LAB]
            for beam_centre in beam_centres_for_lab:
                pos1 = beam_centre.pos1
                pos2 = beam_centre.pos2
                state_builder.set_LAB_sample_centre_pos1(state_builder.convert_pos1(pos1))
                state_builder.set_LAB_sample_centre_pos2(state_builder.convert_pos2(pos2))
                # default both detectors to the same centre position
                if hasattr(state_builder, "set_HAB_sample_centre_pos1"):
                    state_builder.set_HAB_sample_centre_pos1(state_builder.convert_pos1(pos1))
                if hasattr(state_builder, "set_HAB_sample_centre_pos2"):
                    state_builder.set_HAB_sample_centre_pos2(state_builder.convert_pos2(pos2))

        if SetId.CENTRE_HAB in self._input_dict:
            beam_centres = self._input_dict[SetId.CENTRE_HAB]
            beam_centres_for_hab = [beam_centre for beam_centre in beam_centres if beam_centre.detector_type is DetectorType.HAB]
            for beam_centre in beam_centres_for_hab:
                state_builder.set_HAB_sample_centre_pos1(state_builder.convert_pos1(beam_centre.pos1))
                state_builder.set_HAB_sample_centre_pos2(state_builder.convert_pos2(beam_centre.pos2))

        return state_builder.build()

    def get_state_normalize_to_monitor(self, file_information):  # -> StateNormalizeToMonitor:
        builder = get_normalize_to_monitor_builder(self.get_state_data(file_information=file_information))

        if self._all_states:
            builder.state = self._all_states.adjustment.normalize_to_monitor

        state = builder.state
        # Extract the incident monitor and which type of rebinning to use (interpolating or normal)
        if MonId.SPECTRUM in self._input_dict:
            mon_spectrum = self._input_dict[MonId.SPECTRUM]
            mon_spec = [spec for spec in mon_spectrum if not spec.is_trans]

            if mon_spec:
                mon_spec = mon_spec[-1]
                rebin_type = RebinType.INTERPOLATING_REBIN if mon_spec.interpolate else RebinType.REBIN
                state.rebin_type = rebin_type

                #  We have to check if the spectrum is None, this can be the case when the user wants to use the
                # default incident monitor spectrum
                if mon_spec.spectrum:
                    state.incident_monitor = mon_spec.spectrum

        # The prompt peak correction values
        _set_prompt_peak_correction(state, self._input_dict)

        # The general background settings
        _set_background_tof_general(state, self._input_dict)

        # The monitor-specific background settings
        _set_background_tof_monitor(state, self._input_dict)
        return builder.build()

    # We have taken the implementation originally provided, so we can't help the complexity
    def get_state_reduction_mode(self):  # noqa: C901
        state = self._all_states.reduction if self._all_states else StateReductionMode()

        self._set_single_entry(state, "reduction_mode", DetectorId.REDUCTION_MODE)

        # -------------------------------
        # Shift and rescale
        # -------------------------------
        self._set_single_entry(state, "merge_scale", DetectorId.RESCALE)
        self._set_single_entry(state, "merge_shift", DetectorId.SHIFT)

        # -------------------------------
        # User masking
        # -------------------------------
        merge_min = None
        merge_max = None
        merge_mask = False
        if DetectorId.MERGE_RANGE in self._input_dict:
            merge_range = self._input_dict[DetectorId.MERGE_RANGE]
            merge_range = merge_range[-1]
            merge_min = merge_range.start
            merge_max = merge_range.stop
            merge_mask = merge_range.use_fit

        state.merge_mask = merge_mask
        state.merge_min = merge_min
        state.merge_max = merge_max

        # -------------------------------
        # Fitting merged
        # -------------------------------
        q_range_min_scale = None
        q_range_max_scale = None
        has_rescale_fit = False
        if DetectorId.RESCALE_FIT in self._input_dict:
            rescale_fits = self._input_dict[DetectorId.RESCALE_FIT]
            rescale_fit = rescale_fits[-1]
            q_range_min_scale = rescale_fit.start
            q_range_max_scale = rescale_fit.stop
            has_rescale_fit = rescale_fit.use_fit

        q_range_min_shift = None
        q_range_max_shift = None
        has_shift_fit = False
        if DetectorId.SHIFT_FIT in self._input_dict:
            shift_fits = self._input_dict[DetectorId.SHIFT_FIT]
            shift_fit = shift_fits[-1]
            q_range_min_shift = shift_fit.start
            q_range_max_shift = shift_fit.stop
            has_shift_fit = shift_fit.use_fit

        def get_min_q_boundary(min_q1, min_q2):
            if not min_q1 and min_q2:
                val = min_q2
            elif min_q1 and not min_q2:
                val = min_q1
            elif not min_q1 and not min_q2:
                val = None
            else:
                val = max(min_q1, min_q2)
            return val

        def get_max_q_boundary(max_q1, max_q2):
            if not max_q1 and max_q2:
                val = max_q2
            elif max_q1 and not max_q2:
                val = max_q1
            elif not max_q1 and not max_q2:
                val = None
            else:
                val = min(max_q1, max_q2)
            return val

        if has_rescale_fit and has_shift_fit:
            state.merge_fit_mode = FitModeForMerge.BOTH
            min_q = get_min_q_boundary(q_range_min_scale, q_range_min_shift)
            max_q = get_max_q_boundary(q_range_max_scale, q_range_max_shift)
            if min_q:
                state.merge_range_min = min_q
            if max_q:
                state.merge_range_max = max_q
        elif has_rescale_fit and not has_shift_fit:
            state.merge_fit_mode = FitModeForMerge.SCALE_ONLY
            if q_range_min_scale:
                state.merge_range_min = q_range_min_scale
            if q_range_max_scale:
                state.merge_range_max = q_range_max_scale
        elif not has_rescale_fit and has_shift_fit:
            state.merge_fit_mode = FitModeForMerge.SHIFT_ONLY
            if q_range_min_shift:
                state.merge_range_min = q_range_min_shift
            if q_range_max_shift:
                state.merge_range_max = q_range_max_shift
        else:
            state.merge_fit_mode = FitModeForMerge.NO_FIT

        # ------------------------
        # Reduction Dimensionality
        # ------------------------
        self._set_single_entry(state, "reduction_dimensionality", OtherId.REDUCTION_DIMENSIONALITY)
        return state

    def get_state_save(self):
        state = self._all_states.save if self._all_states else StateSave()
        if OtherId.SAVE_TYPES in self._input_dict:
            save_types = self._input_dict[OtherId.SAVE_TYPES]
            save_types = save_types[-1]
            state.file_format = save_types

        if OtherId.SAVE_AS_ZERO_ERROR_FREE in self._input_dict:
            save_as_zero_error_free = self._input_dict[OtherId.SAVE_AS_ZERO_ERROR_FREE]
            save_as_zero_error_free = save_as_zero_error_free[-1]
            state.zero_free_correction = save_as_zero_error_free

        if OtherId.USER_SPECIFIED_OUTPUT_NAME in self._input_dict:
            user_specified_output_name = self._input_dict[OtherId.USER_SPECIFIED_OUTPUT_NAME]
            user_specified_output_name = user_specified_output_name[-1]
            state.user_specified_output_name = user_specified_output_name

        if OtherId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX in self._input_dict:
            user_specified_output_name_suffix = self._input_dict[OtherId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX]
            user_specified_output_name_suffix = user_specified_output_name_suffix[-1]
            state.user_specified_output_name_suffix = user_specified_output_name_suffix

        if OtherId.USE_REDUCTION_MODE_AS_SUFFIX in self._input_dict:
            use_reduction_mode_as_suffix = self._input_dict[OtherId.USE_REDUCTION_MODE_AS_SUFFIX]
            use_reduction_mode_as_suffix = use_reduction_mode_as_suffix[-1]
            state.use_reduction_mode_as_suffix = use_reduction_mode_as_suffix
        return state

    def get_state_scale(self, file_information):
        state = self._all_states.scale if self._all_states else StateScale()

        # We only extract the first entry here, ie the s entry. Although there are other entries which a user can
        # specify such as a, b, c, d they seem to be
        if SetId.SCALES in self._input_dict:
            scales = self._input_dict[SetId.SCALES]
            scales = scales[-1]
            state.scale = scales.s

        # We can also have settings for the sample geometry (Note that at the moment this is not settable via the
        # user file nor the command line interface
        if OtherId.SAMPLE_SHAPE in self._input_dict:
            sample_shape = self._input_dict[OtherId.SAMPLE_SHAPE]
            sample_shape = sample_shape[-1]
            state.shape = sample_shape

        if OtherId.SAMPLE_WIDTH in self._input_dict:
            sample_width = self._input_dict[OtherId.SAMPLE_WIDTH]
            sample_width = sample_width[-1]
            state.width = sample_width

        if OtherId.SAMPLE_HEIGHT in self._input_dict:
            sample_height = self._input_dict[OtherId.SAMPLE_HEIGHT]
            sample_height = sample_height[-1]
            state.height = sample_height

        if OtherId.SAMPLE_THICKNESS in self._input_dict:
            sample_thickness = self._input_dict[OtherId.SAMPLE_THICKNESS]
            sample_thickness = sample_thickness[-1]
            state.thickness = sample_thickness

        if file_information:
            state.set_geometry_from_file(file_information)

        return state

    def get_state_slice_event(self):  # -> StateSliceEvent:
        state = self._all_states.slice if self._all_states else StateSliceEvent()

        # Setting up the slice limits is current
        if OtherId.EVENT_SLICES in self._input_dict:
            event_slices = self._input_dict[OtherId.EVENT_SLICES]
            event_slices = event_slices[-1]
            # The events binning can come in three forms.
            # 1. As a simple range object
            # 2. As an already parsed rebin array, ie min, step, max
            # 3. As a string. Note that this includes custom commands.
            if isinstance(event_slices, simple_range):
                start, stop = get_ranges_for_rebin_setting(event_slices.start, event_slices.stop, event_slices.step, event_slices.step_type)
            elif isinstance(event_slices, rebin_string_values):
                start, stop = get_ranges_for_rebin_array(event_slices.value)
            else:
                pairs = get_ranges_from_event_slice_setting(event_slices.value)
                start = [i[0] for i in pairs]
                stop = [i[1] for i in pairs]

            state.start_time = start
            state.end_time = stop

        return state

    def get_state_wavelength(self):  # -> StateWavelength():
        state = self._all_states.wavelength if self._all_states else StateWavelength()
        _set_wavelength_limits(state, self._input_dict)
        return state

    def get_state_wavelength_and_pixel_adjustment(self):  # -> StateWavelengthAndPixelAdjustment:
        state = self._all_states.adjustment.wavelength_and_pixel_adjustment if self._all_states else StateWavelengthAndPixelAdjustment()
        # Get the flat/flood files. There can be entries for LAB and HAB.
        if MonId.FLAT in self._input_dict:
            mon_flat = self._input_dict[MonId.FLAT]
            hab_flat_entries = [item for item in mon_flat if item.detector_type is DetectorType.HAB]
            lab_flat_entries = [item for item in mon_flat if item.detector_type is DetectorType.LAB]

            if hab_flat_entries:
                hab_flat_entry = hab_flat_entries[-1]
                state.adjustment_files[DetectorType.HAB.value].pixel_adjustment_file = hab_flat_entry.file_path

            if lab_flat_entries:
                lab_flat_entry = lab_flat_entries[-1]
                state.adjustment_files[DetectorType.LAB.value].pixel_adjustment_file = lab_flat_entry.file_path

        # Get the direct files. There can be entries for LAB and HAB.
        if MonId.DIRECT in self._input_dict:
            mon_direct = self._input_dict[MonId.DIRECT]
            hab_direct_entries = [item for item in mon_direct if item.detector_type is DetectorType.HAB]
            lab_direct_entries = [item for item in mon_direct if item.detector_type is DetectorType.LAB]
            if hab_direct_entries:
                hab_direct_entry = hab_direct_entries[-1]
                state.adjustment_files[DetectorType.HAB.value].wavelength_adjustment_file = hab_direct_entry.file_path

            if lab_direct_entries:
                lab_direct_entry = lab_direct_entries[-1]
                state.adjustment_files[DetectorType.LAB.value].wavelength_adjustment_file = lab_direct_entry.file_path

        _set_wavelength_limits(state, self._input_dict)
        return state

    def get_state_polarization(self) -> StatePolarization:
        # The polarization settings are not supported for the legacy txt user file format. Return a blank object.
        return StatePolarization()

    def _set_single_entry(self, state_obj, attr_name, tag, apply_to_value=None):
        """
        Sets a single element on the specified builder via a specified method name.
        If several entries were specified by the user, then the last entry is specified and the
        :param state_obj: a state object
        :param attr_name: the attribute to set on the state
        :param tag: the tag of an entry which is potentially part of all_entries
        :param apply_to_value: a function which should be applied before setting the value. If it is None, then nothing
                               happens
        """
        if tag in self._input_dict:
            list_of_entries = self._input_dict[tag]
            # We expect only one entry, but the user could have specified it several times.
            # We select the entry which was added last.
            entry = list_of_entries[-1]
            if apply_to_value is not None:
                entry = apply_to_value(entry)
            # Set the value on the specified method
            setattr(state_obj, attr_name, entry)


def _convert_mm_to_m(value):
    return value / 1000.0 if value else 0.0


def _get_last_element(val):
    return val[-1] if val else None


def _set_wavelength_limits(state_obj, user_file_items):
    wavelength_limits = _get_last_element(user_file_items.get(LimitsId.WAVELENGTH))

    if not wavelength_limits:
        return state_obj

    if wavelength_limits.step_type in [RangeStepType.RANGE_LIN, RangeStepType.LIN]:
        state_obj.wavelength_step_type = RangeStepType.LIN
    else:
        state_obj.wavelength_step_type = RangeStepType.LOG

    if wavelength_limits.step_type in [RangeStepType.RANGE_LIN, RangeStepType.RANGE_LOG]:
        wavelength_range = _get_last_element(user_file_items.get(OtherId.WAVELENGTH_RANGE))
        full_interval, pairs = parse_range_wavelength(wavelength_range)

        state_obj.wavelength_interval.wavelength_full_range = full_interval
        state_obj.wavelength_interval.selected_ranges = pairs
        state_obj.wavelength_interval.wavelength_step = wavelength_limits.step
    else:
        full_range = (wavelength_limits.start, wavelength_limits.stop)
        state_obj.wavelength_interval.wavelength_full_range = full_range
        state_obj.wavelength_interval.wavelength_step = wavelength_limits.step

    return state_obj


def _set_prompt_peak_correction(state_obj, user_file_items):
    fit_monitor_times = _get_last_element(user_file_items.get(FitId.MONITOR_TIMES))
    if fit_monitor_times:
        state_obj.prompt_peak_correction_min = fit_monitor_times.start
        state_obj.prompt_peak_correction_max = fit_monitor_times.stop


def _set_background_tof_general(state_obj, user_file_items):
    # The general background settings
    back_all_monitors = _get_last_element(user_file_items.get(BackId.ALL_MONITORS))
    if back_all_monitors:
        state_obj.background_TOF_general_start = back_all_monitors.start
        state_obj.background_TOF_general_stop = back_all_monitors.stop


def _set_background_tof_monitor(state_obj, user_file_items):
    # The monitor off switches. Get all monitors which should not have an individual background setting
    monitor_exclusion_list = []

    back_monitor_off = user_file_items.get(BackId.MONITOR_OFF)
    if back_monitor_off:
        monitor_exclusion_list = list(back_monitor_off.values())

    # Get all individual monitor background settings. But ignore those settings where there was an explicit
    # off setting. Those monitors were collected in the monitor_exclusion_list collection
    back_single_monitors = user_file_items.get(BackId.SINGLE_MONITORS)
    if back_single_monitors:
        background_tof_monitor_start = {}
        background_tof_monitor_stop = {}

        for element in back_single_monitors:
            monitor = element.monitor
            if monitor not in monitor_exclusion_list:
                background_tof_monitor_start.update({str(monitor): element.start})
                background_tof_monitor_stop.update({str(monitor): element.stop})

        state_obj.background_TOF_monitor_start = background_tof_monitor_start
        state_obj.background_TOF_monitor_stop = background_tof_monitor_stop
