from mantid.kernel import logger

from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSEnumerations import (DetectorType, FitModeForMerge)
from SANS2.Common.SANSFileInformation import find_full_file_path
from SANS2.UserFile.UserFileReader import UserFileReader
from SANS2.UserFile.UserFileCommon import *  # noqa

from SANS2.State.StateBuilder.SANSStateBuilder import get_state_builder
from SANS2.State.StateBuilder.SANSStateMaskBuilder import get_mask_builder
from SANS2.State.StateBuilder.SANSStateMoveBuilder import get_move_builder
from SANS2.State.StateBuilder.SANSStateReductionBuilder import get_reduction_builder
from SANS2.State.StateBuilder.SANSStateSliceEventBuilder import get_slice_event_builder
from SANS2.State.StateBuilder.SANSStateWavelengthBuilder import get_wavelength_builder
from SANS2.State.StateBuilder.SANSStateSaveBuilder import get_save_builder


def check_if_contains_only_one_element(to_check, element_name):
    if len(to_check) > 1:
        msg = "The element {0} contains more than one element. Expected only one element. " \
              "The last element {1} is used. The elements {2} are discarded.".format(element_name,
                                                                                     to_check[-1], to_check[:-1])
        logger.notice(msg)


def log_non_existing_field(field):
    msg = "The field {0} does not seem to exist on the state.".format(field)
    logger.notice(msg)


def convert_detector(detector_type):
    if detector_type is DetectorType.Hab:
        detector_type_as_string = SANSConstants.high_angle_bank
    elif detector_type is DetectorType.Lab:
        detector_type_as_string = SANSConstants.low_angle_bank
    else:
        raise RuntimeError("UserFileStateDirector: Cannot convert detector {0}".format(detector_type))
    return detector_type_as_string


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


def convert_mm_to_m(value):
    return value/1000.


class UserFileStateDirectorISIS(object):
    def __init__(self, data_info):
        super(UserFileStateDirectorISIS, self).__init__()
        data_info.validate()
        self._data = data_info

        self._user_file = None

        self._state_builder = get_state_builder(self._data)
        self._mask_builder = get_mask_builder(self._data)
        self._move_builder = get_move_builder(self._data)
        self._reduction_builder = get_reduction_builder(self._data)
        self._slice_event_builder = get_slice_event_builder(self._data)
        self._wavelength_builder = get_wavelength_builder(self._data)
        self._save_builder = get_save_builder(self._data)

    def set_user_file(self, user_file):
        file_path = find_full_file_path(user_file)
        if file_path is None:
            raise RuntimeError("UserFileStateDirector: The specified user file cannot be found. Make sure that the "
                               "directory which contains the user file is added to the Mantid path.")
        self._user_file = file_path
        reader = UserFileReader(self._user_file)
        user_file_items = reader.read_user_file()
        # ----------------------------------------------------
        # Populate the different sub states from the user file
        # ----------------------------------------------------
        # Mask state
        self._set_up_mask_state(user_file_items)

        # Reduction state
        self._set_up_reduction_state(user_file_items)

        # Move state
        self._set_up_move_state(user_file_items)

        # Wavelength state
        self._set_up_wavelength_state(user_file_items)

        # Slice event state
        # There does not seem to be a command for this currently -- this should be added in the future

    def construct(self):
        # Create the different sub states and add them to the state
        # Mask state
        mask_state = self._mask_builder.build()
        mask_state.validate()
        self._state_builder.set_mask(mask_state)

        # Reduction state
        reduction_state = self._reduction_builder.build()
        reduction_state.validate()
        self._state_builder.set_reduction(reduction_state)

        # Move state
        move_state = self._move_builder.build()
        move_state.validate()
        self._state_builder.set_move(move_state)

        # Slice Event state
        slice_event_state = self._slice_event_builder.build()
        slice_event_state.validate()
        self._state_builder.set_slice(slice_event_state)

        # Wavelength conversion state
        wavelength_state = self._wavelength_builder.build()
        wavelength_state.validate()
        self._state_builder.set_wavelength(wavelength_state)

        # Save state
        save_state = self._save_builder.build()
        save_state.validate()
        self._state_builder.set_save(save_state)

        # Data state
        self._state_builder.set_data(self._data)

        return self._state_builder.build()

    def _set_up_move_state(self, user_file_items):  # noqa
        # The elements which can be set up via the user file are:
        # 1. Correction in X, Y, Z
        # 2. Rotation
        # 3. Side translation
        # 4. Xtilt and Ytilt
        # 5. Sample offset
        # 6. Monitor 4 offset
        # 7. Beam centre

        # ---------------------------
        # Correction for X, Y, Z
        # ---------------------------
        if user_file_det_correction_x in user_file_items:
            corrections_in_x = user_file_items[user_file_det_correction_x]
            for correction_x in corrections_in_x:
                if correction_x.detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_x_translation_correction(convert_mm_to_m(correction_x.entry))
                elif correction_x.detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_x_translation_correction(convert_mm_to_m(correction_x.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " x correction.".format(correction_x.detector_type))

        if user_file_det_correction_y in user_file_items:
            corrections_in_y = user_file_items[user_file_det_correction_y]
            for correction_y in corrections_in_y:
                if correction_y.detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_y_translation_correction(convert_mm_to_m(correction_y.entry))
                elif correction_y.detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_y_translation_correction(convert_mm_to_m(correction_y.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " y correction.".format(correction_y.detector_type))

        if user_file_det_correction_z in user_file_items:
            corrections_in_z = user_file_items[user_file_det_correction_z]
            for correction_z in corrections_in_z:
                if correction_z.detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_z_translation_correction(convert_mm_to_m(correction_z.entry))
                elif correction_z.detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_z_translation_correction(convert_mm_to_m(correction_z.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " z correction.".format(correction_z.detector_type))

        # ---------------------------
        # Correction for Rotation
        # ---------------------------
        if user_file_det_correction_rotation in user_file_items:
            rotation_correction = user_file_items[user_file_det_correction_rotation]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(rotation_correction, user_file_det_correction_rotation)
            rotation_correction = rotation_correction[-1]
            if rotation_correction.detector_type is DetectorType.Hab:
                self._move_builder.set_HAB_rotation_correction(rotation_correction.entry)
            elif rotation_correction.detector_type is DetectorType.Lab:
                self._move_builder.set_LAB_rotation_correction(rotation_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " rotation correction.".format(rotation_correction.detector_type))

        # ---------------------------
        # Correction for Radius
        # ---------------------------
        if user_file_det_correction_radius in user_file_items:
            radius_corrections = user_file_items[user_file_det_correction_radius]
            for radius_correction in radius_corrections:
                if radius_correction.detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_radius_correction(convert_mm_to_m(radius_correction.entry))
                elif radius_correction.detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_radius_correction(convert_mm_to_m(radius_correction.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " radius correction.".format(radius_correction.detector_type))

        # ---------------------------
        # Correction for Translation
        # ---------------------------
        if user_file_det_correction_translation in user_file_items:
            side_corrections = user_file_items[user_file_det_correction_translation]
            for side_correction in side_corrections:
                if side_correction.detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_side_correction(convert_mm_to_m(side_correction.entry))
                elif side_correction.detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_side_correction(convert_mm_to_m(side_correction.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " side correction.".format(side_correction.detector_type))

        # ---------------------------
        # Tilt
        # ---------------------------
        if user_file_det_correction_x_tilt in user_file_items:
            tilt_correction = user_file_items[user_file_det_correction_x_tilt]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.Hab:
                self._move_builder.set_HAB_x_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.Lab:
                self._move_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " titlt correction.".format(tilt_correction.detector_type))

        if user_file_det_correction_y_tilt in user_file_items:
            tilt_correction = user_file_items[user_file_det_correction_y_tilt]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.Hab:
                self._move_builder.set_HAB_y_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.Lab:
                self._move_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " titlt correction.".format(tilt_correction.detector_type))

        # ---------------------------
        # Sample offset
        # ---------------------------
        if user_file_sample_offset in user_file_items:
            sample_offset = user_file_items[user_file_sample_offset]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(sample_offset, user_file_sample_offset)
            sample_offset = sample_offset[-1]
            self._move_builder.set_sample_offset(convert_mm_to_m(sample_offset))

        # ---------------------------
        # Monitor 4 offset; for now this is only SANS2D
        # ---------------------------
        if user_file_trans_spec_shift in user_file_items:
            monitor_4_shift = user_file_items[user_file_trans_spec_shift]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(monitor_4_shift, user_file_trans_spec_shift)
            monitor_4_shift = monitor_4_shift[-1]
            set_monitor_4_offset = getattr(self._move_builder, "set_monitor_4_offset", None)
            if callable(set_monitor_4_offset):
                self._move_builder.set_monitor_4_offset(convert_mm_to_m(monitor_4_shift))
            else:
                log_non_existing_field("set_monitor_4_offset")

        # ---------------------------
        # Beam Centre, this can be for HAB and LAB
        # ---------------------------
        if user_file_set_centre in user_file_items:
            beam_centres = user_file_items[user_file_set_centre]
            for beam_centre in beam_centres:
                detector_type = beam_centre.detector_type
                pos1 = beam_centre.pos1
                pos2 = beam_centre.pos2
                if detector_type is DetectorType.Hab:
                    self._move_builder.set_HAB_sample_centre_pos1(self._move_builder.convert_pos1(pos1))
                    self._move_builder.set_HAB_sample_centre_pos2(self._move_builder.convert_pos2(pos2))
                elif detector_type is DetectorType.Lab:
                    self._move_builder.set_LAB_sample_centre_pos1(self._move_builder.convert_pos1(pos1))
                    self._move_builder.set_LAB_sample_centre_pos2(self._move_builder.convert_pos2(pos2))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " beam centre.".format(beam_centre.detector_type))

    def _set_up_reduction_state(self, user_file_items):
        # There are several things that can be extracted from the user file
        # 1. The reduction mode
        # 2. The merge behaviour
        # 3. The dimensionality is not set via the user file

        # ------------------------
        # Reduction mode
        # ------------------------
        if user_file_det_reduction_mode in user_file_items:
            reduction_modes = user_file_items[user_file_det_reduction_mode]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(reduction_modes, user_file_det_reduction_mode)
            reduction_mode = reduction_modes[-1]
            self._reduction_builder.set_reduction_mode(reduction_mode)

        # -------------------------------
        # Shift and rescale
        # -------------------------------
        if user_file_det_rescale in user_file_items:
            rescales = user_file_items[user_file_det_rescale]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(rescales, user_file_det_rescale)
            rescale = rescales[-1]
            self._reduction_builder.set_merge_rescale(rescale)

        if user_file_det_shift in user_file_items:
            shifts = user_file_items[user_file_det_shift]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(shifts, user_file_det_shift)
            shift = shifts[-1]
            self._reduction_builder.set_merge_shift(shift)

        # -------------------------------
        # Fitting merged
        # -------------------------------
        q_range_min_scale = None
        q_range_max_scale = None
        has_rescale_fit = False
        if user_file_det_rescale_fit in user_file_items:
            rescale_fits = user_file_items[user_file_det_rescale_fit]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(rescale_fits, user_file_det_rescale_fit)
            rescale_fit = rescale_fits[-1]
            q_range_min_scale = rescale_fit.start
            q_range_max_scale = rescale_fit.stop
            has_rescale_fit = True

        q_range_min_shift = None
        q_range_max_shift = None
        has_shift_fit = False
        if user_file_det_shift_fit in user_file_items:
            shift_fits = user_file_items[user_file_det_shift_fit]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(shift_fits, user_file_det_shift_fit)
            shift_fit = shift_fits[-1]
            q_range_min_shift = shift_fit.start
            q_range_max_shift = shift_fit.stop
            has_shift_fit = True

        if has_rescale_fit and has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.Both)
            min_q = get_min_q_boundary(q_range_min_scale, q_range_min_shift)
            max_q = get_max_q_boundary(q_range_max_scale, q_range_max_shift)
            self._reduction_builder.set_merge_range_min(min_q)
            self._reduction_builder.set_merge_range_max(max_q)
        elif has_rescale_fit and not has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.ScaleOnly)
            self._reduction_builder.set_merge_range_min(q_range_min_scale)
            self._reduction_builder.set_merge_range_max(q_range_max_scale)
        elif not has_rescale_fit and has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.ShiftOnly)
            self._reduction_builder.set_merge_range_min(q_range_min_shift)
            self._reduction_builder.set_merge_range_max(q_range_max_shift)
        else:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.None)
            self._reduction_builder.set_merge_range_min(None)
            self._reduction_builder.set_merge_range_max(None)

    def _set_up_mask_state(self, user_file_items):  # noqa
        # Check for the various possible masks that can be present in the user file
        # This can be:
        # 1. A line mask
        # 2. A time mask
        # 3. A detector-bound time mask
        # 4. A clean command
        # 5. A time clear command
        # 6. A single spectrum mask
        # 7. A spectrum range mask
        # 8. A vertical single strip mask
        # 9. A vertical range strip mask
        # 10. A horizontal single strip mask
        # 11. A horizontal range strip mask
        # 12. A block mask
        # 13. A cross-type block mask
        # 14. Angle masking
        # 15. Mask files

        # ---------------------------------
        # 1. Line Mask
        # ---------------------------------
        if user_file_mask_line in user_file_items:
            mask_lines = user_file_items[user_file_mask_line]
            # If there were several arms specified then we take only the last
            check_if_contains_only_one_element(mask_lines, user_file_mask_line)
            mask_line = mask_lines[-1]
            # We need the width and the angle
            angle = mask_line.angle
            width = convert_mm_to_m(mask_line.width)
            # The position is already specified in meters in the user file
            pos1 = mask_line.x
            pos2 = mask_line.y
            if angle is None or width is None:
                raise RuntimeError("UserFileStateDirector: You specified a line mask without an angle or a width."
                                   "The parameters were: width {0}; angle {1}; x {2}; y {3}".format(width, angle,
                                                                                                    pos1, pos2))
            pos1 = 0.0 if pos1 is None else pos1
            pos2 = 0.0 if pos2 is None else pos2

            self._mask_builder.set_beam_stop_arm_width(width)
            self._mask_builder.set_beam_stop_arm_angle(angle)
            self._mask_builder.set_beam_stop_arm_pos1(pos1)
            self._mask_builder.set_beam_stop_arm_pos2(pos2)

        # ---------------------------------
        # 2. General time mask
        # ---------------------------------
        if user_file_mask_time in user_file_items:
            mask_time_general = user_file_items[user_file_mask_time]
            start_time = []
            stop_time =[]
            for times in mask_time_general:
                if times.start > times.start:
                    raise RuntimeError("UserFileStateDirector: You specified a general time mask with a start time {0}"
                                       " which is larger than the stop time {1} of the mask. This is not"
                                       " valid.".format(times.start, times.stop))
                start_time.append(times.start)
                stop_time.append(times.stop)
            self._mask_builder.set_bin_mask_general_start(start_time)
            self._mask_builder.set_bin_mask_general_stop(stop_time)

        # ---------------------------------
        # 3. Detector-bound time mask
        # ---------------------------------
        if user_file_mask_time_detector in user_file_items:
            mask_times = user_file_items[user_file_mask_time_detector]
            start_times_hab = []
            stop_times_hab = []
            start_times_lab = []
            stop_times_lab = []
            for times in mask_times:
                if times.start > times.start:
                    raise RuntimeError("UserFileStateDirector: You specified a general time mask with a start time {0}"
                                       " which is larger than the stop time {1} of the mask. This is not"
                                       " valid.".format(times.start, times.stop))
                if times.detector_type is DetectorType.Hab:
                    start_times_hab.append(times.start)
                    stop_times_hab.append(times.stop)
                elif times.detector_type is DetectorType.Lab:
                    start_times_hab.append(times.start)
                    stop_times_hab.append(times.stop)
                else:
                    RuntimeError("UserFileStateDirector: The specified detector {0} is not "
                                 "known".format(times.detector_type))
            self._mask_builder.set_HAB_bin_mask_start(start_times_hab)
            self._mask_builder.set_HAB_bin_mask_stop(stop_times_hab)
            self._mask_builder.set_LAB_bin_mask_start(start_times_lab)
            self._mask_builder.set_LAB_bin_mask_stop(stop_times_lab)

        # ---------------------------------
        # 4. Clear detector
        # ---------------------------------
        if user_file_mask_clear_detector_mask in user_file_items:
            clear_detectors = user_file_items[user_file_mask_clear_detector_mask]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(clear_detectors, user_file_mask_clear_detector_mask)
            clear_detector = clear_detectors[-1]
            self._mask_builder.set_clear(clear_detector)

        # ---------------------------------
        # 5. Clear time
        # ---------------------------------
        if user_file_mask_clear_time_mask in user_file_items:
            clear_times = user_file_items[user_file_mask_clear_time_mask]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(clear_times, user_file_mask_clear_time_mask)
            clear_time = clear_times[-1]
            self._mask_builder.set_clear_time(clear_time)

        # ---------------------------------
        # 6. Single Spectrum
        # ---------------------------------
        if user_file_mask_single_spectrum_mask in user_file_items:
            single_spectra = user_file_items[user_file_mask_single_spectrum_mask]
            self._mask_builder.set_single_spectra(single_spectra)

        # ---------------------------------
        # 7. Spectrum Range
        # ---------------------------------
        if user_file_mask_spectrum_range_mask in user_file_items:
            spectrum_ranges = user_file_items[user_file_mask_spectrum_range_mask]
            start_range = []
            stop_range = []
            for spectrum_range in spectrum_ranges:
                if spectrum_range.start > spectrum_range.start:
                    raise RuntimeError("UserFileStateDirector: You specified a spectrum range with a start value {0}"
                                       " which is larger than the stop value {1}. This is not"
                                       " valid.".format(spectrum_range.start, spectrum_range.stop))
                start_range.append(spectrum_range.start)
                stop_range.append(spectrum_range.stop)
            self._mask_builder.set_spectrum_range_start(start_range)
            self._mask_builder.set_spectrum_range_stop(stop_range)

        # ---------------------------------
        # 8. Vertical single strip
        # ---------------------------------
        if user_file_mask_vertical_single_strip_mask in user_file_items:
            single_vertical_strip_masks = user_file_items[user_file_mask_vertical_single_strip_mask]
            entry_hab = []
            entry_lab = []
            for single_vertical_strip_mask in single_vertical_strip_masks:
                if single_vertical_strip_mask.detector_type is DetectorType.Hab:
                    entry_hab.append(single_vertical_strip_mask.entry)
                elif single_vertical_strip_mask.detector_type is DetectorType.Lab:
                    entry_lab.append(single_vertical_strip_mask.entry)
                else:
                    raise RuntimeError("UserFileStateDirector: The vertical single strip mask {0} has an unknown "
                                       "detector {1} associated"
                                       " with it.".format(single_vertical_strip_mask.entry,
                                                          single_vertical_strip_mask.detector_type))
            self._mask_builder.set_HAB_single_vertical_strip_mask(entry_hab)
            self._mask_builder.set_LAB_single_vertical_strip_mask(entry_lab)

        # ---------------------------------
        # 9. Vertical range strip
        # ---------------------------------
        if user_file_mask_vertical_range_strip_mask in user_file_items:
            range_vertical_strip_masks = user_file_items[user_file_mask_vertical_range_strip_mask]
            start_hab = []
            stop_hab = []
            start_lab = []
            stop_lab = []
            for range_vertical_strip_mask in range_vertical_strip_masks:
                if range_vertical_strip_mask.detector_type is DetectorType.Hab:
                    start_hab.append(range_vertical_strip_mask.start)
                    stop_hab.append(range_vertical_strip_mask.stop)
                elif range_vertical_strip_mask.detector_type is DetectorType.Lab:
                    start_lab.append(range_vertical_strip_mask.start)
                    stop_lab.append(range_vertical_strip_mask.stop)
                else:
                    raise RuntimeError("UserFileStateDirector: The vertical range strip mask {0} has an unknown "
                                       "detector {1} associated "
                                       "with it.".format(range_vertical_strip_mask.entry,
                                                         range_vertical_strip_mask.detector_type))
            self._mask_builder.set_HAB_range_vertical_strip_start(start_hab)
            self._mask_builder.set_HAB_range_vertical_strip_stop(stop_hab)
            self._mask_builder.set_LAB_range_vertical_strip_start(start_lab)
            self._mask_builder.set_LAB_range_vertical_strip_stop(stop_lab)

        # ---------------------------------
        # 10. Horizontal single strip
        # ---------------------------------
        if user_file_mask_horizontal_single_strip_mask in user_file_items:
            single_horizontal_strip_masks = user_file_items[user_file_mask_horizontal_single_strip_mask]
            entry_hab = []
            entry_lab = []
            for single_horizontal_strip_mask in single_horizontal_strip_masks:
                if single_horizontal_strip_mask.detector_type is DetectorType.Hab:
                    entry_hab.append(single_horizontal_strip_mask.entry)
                elif single_horizontal_strip_mask.detector_type is DetectorType.Lab:
                    entry_lab.append(single_horizontal_strip_mask.entry)
                else:
                    raise RuntimeError("UserFileStateDirector: The horizontal single strip mask {0} has an unknown "
                                       "detector {1} associated"
                                       " with it.".format(single_horizontal_strip_mask.entry,
                                                          single_horizontal_strip_mask.detector_type))
            self._mask_builder.set_HAB_single_horizontal_strip_mask(entry_hab)
            self._mask_builder.set_LAB_single_horizontal_strip_mask(entry_lab)

        # ---------------------------------
        # 11. Horizontal range strip
        # ---------------------------------
        if user_file_mask_horizontal_range_strip_mask in user_file_items:
            range_horizontal_strip_masks = user_file_items[user_file_mask_horizontal_range_strip_mask]
            start_hab = []
            stop_hab = []
            start_lab = []
            stop_lab = []
            for range_horizontal_strip_mask in range_horizontal_strip_masks:
                if range_horizontal_strip_mask.detector_type is DetectorType.Hab:
                    start_hab.append(range_horizontal_strip_mask.start)
                    stop_hab.append(range_horizontal_strip_mask.stop)
                elif range_horizontal_strip_mask.detector_type is DetectorType.Lab:
                    start_lab.append(range_horizontal_strip_mask.start)
                    stop_lab.append(range_horizontal_strip_mask.stop)
                else:
                    raise RuntimeError("UserFileStateDirector: The vertical range strip mask {0} has an unknown "
                                       "detector {1} associated "
                                       "with it.".format(range_horizontal_strip_mask.entry,
                                                         range_horizontal_strip_mask.detector_type))
            self._mask_builder.set_HAB_range_horizontal_strip_start(start_hab)
            self._mask_builder.set_HAB_range_horizontal_strip_stop(stop_hab)
            self._mask_builder.set_LAB_range_horizontal_strip_start(start_lab)
            self._mask_builder.set_LAB_range_horizontal_strip_stop(stop_lab)

        # ---------------------------------
        # 12. Block
        # ---------------------------------
        if user_file_mask_block in user_file_items:
            blocks = user_file_items[user_file_mask_block]
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
                    raise RuntimeError("UserFileStateDirector: The block mask seems to have inconsistent entries. "
                                       "The values are horizontal_start {0}; horizontal_stop {1}; vertical_start {2};"
                                       " vertical_stop {3}".format(block.horizontal1, block.horizontal2,
                                                                   block.vertical1, block.vertical2))
                if block.detector_type is DetectorType.Hab:
                    horizontal_start_hab.append(block.horizontal1)
                    horizontal_stop_hab.append(block.horizontal2)
                    vertical_start_hab.append(block.vertical1)
                    vertical_stop_hab.append(block.vertical2)
                elif block.detector_type is DetectorType.Lab:
                    horizontal_start_lab.append(block.horizontal1)
                    horizontal_stop_lab.append(block.horizontal2)
                    vertical_start_lab.append(block.vertical1)
                    vertical_stop_lab.append(block.vertical2)
                else:
                    raise RuntimeError("UserFileStateDirector: The block mask has an unknown "
                                       "detector {0} associated "
                                       "with it.".format(block.detector_type))
            self._mask_builder.set_HAB_block_horizontal_start(horizontal_start_hab)
            self._mask_builder.set_HAB_block_horizontal_stop(horizontal_start_hab)
            self._mask_builder.set_LAB_block_vertical_start(vertical_start_lab)
            self._mask_builder.set_LAB_block_vertical_stop(vertical_start_lab)

        # ---------------------------------
        # 13. Block cross
        # ---------------------------------
        if user_file_mask_block_cross in user_file_items:
            block_crosses = user_file_items[user_file_mask_block_cross]
            horizontal_hab = []
            vertical_hab = []
            horizontal_lab = []
            vertical_lab = []
            for block_cross in block_crosses:
                if block_cross.detector_type is DetectorType.Hab:
                    horizontal_hab.append(block_cross.horizontal)
                    vertical_hab.append(block_cross.vertical)
                elif block_cross.detector_type is DetectorType.Lab:
                    horizontal_lab.append(block_cross.horizontal)
                    vertical_lab.append(block_cross.vertical)
                else:
                    raise RuntimeError("UserFileStateDirector: The block cross mask has an unknown "
                                       "detector {0} associated "
                                       "with it.".format(block_cross.detector_type))
            self._mask_builder.set_HAB_block_cross_horizontal(horizontal_hab)
            self._mask_builder.set_HAB_block_cross_vertical(vertical_hab)
            self._mask_builder.set_LAB_block_cross_horizontal(horizontal_lab)
            self._mask_builder.set_LAB_block_cross_vertical(vertical_lab)

        # ------------------------------------------------------------
        # 14. Angles --> they are specified in L/Phi
        # -----------------------------------------------------------
        if user_file_limits_angle in user_file_items:
            angles = user_file_items[user_file_limits_angle]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(angles, user_file_limits_angle)
            angle = angles[-1]
            self._mask_builder.set_phi_min(angle.min)
            self._mask_builder.set_phi_max(angle.max)
            self._mask_builder.set_use_mask_phi_mirror(not angle.is_no_mirror)

        # ------------------------------------------------------------
        # 15. Maskfiles
        # -----------------------------------------------------------
        if user_file_mask_file in user_file_items:
            mask_files = user_file_items[user_file_mask_file]
            self._mask_builder.set_mask_files(mask_files)

        # ------------------------------------------------------------
        # 16. Radius masks
        # -----------------------------------------------------------
        if user_file_limits_radius in user_file_items:
            radii = user_file_items[user_file_limits_radius]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(radii, user_file_limits_radius)
            radius = radii[-1]
            if radius.start > 0 and radius.stop > 0 and radius.start > radius.stop:
                raise RuntimeError("UserFileStateDirector: The inner radius {0} appears to be larger that the outer"
                                   " radius {1} of the mask.".format(radius.start, radius.stop))
            self._mask_builder.set_radius_min(convert_mm_to_m(radius.start))
            self._mask_builder.set_radius_max(convert_mm_to_m(radius.stop))

    def _set_up_wavelength_state(self, user_file_items):
        if user_file_limits_wavelength in user_file_items:
            wavelength_limits = user_file_items[user_file_limits_wavelength]
            check_if_contains_only_one_element(wavelength_limits, user_file_limits_wavelength)
            wavelength_limits = wavelength_limits[-1]
            self._wavelength_builder.set_wavelength_low(wavelength_limits.start)
            self._wavelength_builder.set_wavelength_high(wavelength_limits.stop)
            self._wavelength_builder.set_wavelength_step(wavelength_limits.step)
            self._wavelength_builder.set_wavelength_step_type(wavelength_limits.step_type)
