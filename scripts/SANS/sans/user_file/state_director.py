# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import logger

from sans.common.enums import (DetectorType, FitModeForMerge, RebinType, DataType, FitType, RangeStepType)
from sans.common.file_information import find_full_file_path
from sans.common.general_functions import (get_ranges_for_rebin_setting, get_ranges_for_rebin_array,
                                           get_ranges_from_event_slice_setting)
from sans.user_file.user_file_reader import UserFileReader
from sans.user_file.settings_tags import (DetectorId, BackId, LimitsId, simple_range, complex_range, MaskId,
                                          rebin_string_values, SampleId, SetId, TransId, TubeCalibrationFileId,
                                          QResolutionId, FitId, MonId, GravityId, OtherId)
from sans.state.automatic_setters import set_up_setter_forwarding_from_director_to_builder
from sans.state.state import get_state_builder
from sans.state.mask import get_mask_builder
from sans.state.move import get_move_builder
from sans.state.reduction_mode import get_reduction_mode_builder
from sans.state.slice_event import get_slice_event_builder
from sans.state.wavelength import get_wavelength_builder
from sans.state.save import get_save_builder
from sans.state.scale import get_scale_builder
from sans.state.adjustment import get_adjustment_builder
from sans.state.normalize_to_monitor import get_normalize_to_monitor_builder
from sans.state.calculate_transmission import get_calculate_transmission_builder
from sans.state.wavelength_and_pixel_adjustment import get_wavelength_and_pixel_adjustment_builder
from sans.state.convert_to_q import get_convert_to_q_builder
from sans.state.compatibility import get_compatibility_builder
import collections


def check_if_contains_only_one_element(to_check, element_name):
    if len(to_check) > 1:
        msg = "The element {0} contains more than one element. Expected only one element. " \
              "The last element {1} is used. The elements {2} are discarded.".format(element_name,
                                                                                     to_check[-1], to_check[:-1])
        logger.information(msg)


def log_non_existing_field(field):
    msg = "The field {0} does not seem to exist on the state.".format(field)
    logger.information(msg)


def convert_detector(detector_type):
    if detector_type is DetectorType.HAB:
        detector_type_as_string = DetectorType.HAB.name
    elif detector_type is DetectorType.LAB:
        detector_type_as_string = DetectorType.LAB.name
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


def set_background_tof_general(builder, user_file_items):
    # The general background settings
    if BackId.all_monitors in user_file_items:
        back_all_monitors = user_file_items[BackId.all_monitors]
        # Should the user have chosen several values, then the last element is selected
        check_if_contains_only_one_element(back_all_monitors, BackId.all_monitors)
        back_all_monitors = back_all_monitors[-1]
        builder.set_background_TOF_general_start(back_all_monitors.start)
        builder.set_background_TOF_general_stop(back_all_monitors.stop)


def set_background_tof_monitor(builder, user_file_items):
    # The monitor off switches. Get all monitors which should not have an individual background setting
    monitor_exclusion_list = []
    if BackId.monitor_off in user_file_items:
        back_monitor_off = user_file_items[BackId.monitor_off]
        monitor_exclusion_list = list(back_monitor_off.values())

    # Get all individual monitor background settings. But ignore those settings where there was an explicit
    # off setting. Those monitors were collected in the monitor_exclusion_list collection
    if BackId.single_monitors in user_file_items:
        background_tof_monitor_start = {}
        background_tof_monitor_stop = {}
        back_single_monitors = user_file_items[BackId.single_monitors]
        for element in back_single_monitors:
            monitor = element.monitor
            if monitor not in monitor_exclusion_list:
                # We need to set it to string since Mantid's Property manager cannot handle integers as a key.
                background_tof_monitor_start.update({str(monitor): element.start})
                background_tof_monitor_stop.update({str(monitor): element.stop})
        builder.set_background_TOF_monitor_start(background_tof_monitor_start)
        builder.set_background_TOF_monitor_stop(background_tof_monitor_stop)


def set_wavelength_limits(builder, user_file_items):
    if LimitsId.wavelength in user_file_items:
        wavelength_limits = user_file_items[LimitsId.wavelength]
        check_if_contains_only_one_element(wavelength_limits, LimitsId.wavelength)
        wavelength_limits = wavelength_limits[-1]

        if wavelength_limits.step_type in [RangeStepType.RangeLin, RangeStepType.RangeLog]:
            wavelength_range = user_file_items[OtherId.wavelength_range]
            check_if_contains_only_one_element(wavelength_range, OtherId.wavelength_range)
            wavelength_range = wavelength_range[-1]
            wavelength_start, wavelength_stop = get_ranges_from_event_slice_setting(wavelength_range)
            wavelength_start = [min(wavelength_start)] + wavelength_start
            wavelength_stop = [max(wavelength_stop)] + wavelength_stop

            wavelength_step_type = RangeStepType.Lin if wavelength_limits.step_type is RangeStepType.RangeLin \
                else RangeStepType.Log

            builder.set_wavelength_low(wavelength_start)
            builder.set_wavelength_high(wavelength_stop)
            builder.set_wavelength_step(wavelength_limits.step)
            builder.set_wavelength_step_type(wavelength_step_type)
        else:
            builder.set_wavelength_low([wavelength_limits.start])
            builder.set_wavelength_high([wavelength_limits.stop])
            builder.set_wavelength_step(wavelength_limits.step)
            builder.set_wavelength_step_type(wavelength_limits.step_type)


def set_prompt_peak_correction(builder, user_file_items):
    if FitId.monitor_times in user_file_items:
        fit_monitor_times = user_file_items[FitId.monitor_times]
        # Should the user have chosen several values, then the last element is selected
        check_if_contains_only_one_element(fit_monitor_times, FitId.monitor_times)
        fit_monitor_times = fit_monitor_times[-1]
        builder.set_prompt_peak_correction_min(fit_monitor_times.start)
        builder.set_prompt_peak_correction_max(fit_monitor_times.stop)


def set_single_entry(builder, method_name, tag, all_entries, apply_to_value=None):
    """
    Sets a single element on the specified builder via a specified method name.

    If several entries were specified by the user, then the last entry is specified and the
    :param builder: a builder object
    :param method_name: a method on the builder object
    :param tag: the tag of an entry which is potentially part of all_entries
    :param all_entries: all parsed entries
    :param apply_to_value: a function which should be applied before setting the value. If it is None, then nothing
                           happens
    """
    if tag in all_entries:
        list_of_entries = all_entries[tag]
        # We expect only one entry, but the user could have specified it several times.
        # If so we want to log it.
        check_if_contains_only_one_element(list_of_entries, tag)
        # We select the entry which was added last.
        entry = list_of_entries[-1]
        if apply_to_value is not None:
            entry = apply_to_value(entry)
        # Set the value on the specified method
        method = getattr(builder, method_name)
        method(entry)


class StateDirectorISIS(object):
    def __init__(self, data_info, file_information):
        super(StateDirectorISIS, self).__init__()
        data_info.validate()
        self._data = data_info

        self._user_file = None

        self._state_builder = get_state_builder(self._data)
        self._mask_builder = get_mask_builder(self._data)
        self._move_builder = get_move_builder(self._data)
        self._reduction_builder = get_reduction_mode_builder(self._data)
        self._slice_event_builder = get_slice_event_builder(self._data)
        self._wavelength_builder = get_wavelength_builder(self._data)
        self._save_builder = get_save_builder(self._data)
        self._scale_builder = get_scale_builder(self._data, file_information)

        self._adjustment_builder = get_adjustment_builder(self._data)
        self._normalize_to_monitor_builder = get_normalize_to_monitor_builder(self._data)
        self._calculate_transmission_builder = get_calculate_transmission_builder(self._data)
        self._wavelength_and_pixel_adjustment_builder = get_wavelength_and_pixel_adjustment_builder(self._data)

        self._convert_to_q_builder = get_convert_to_q_builder(self._data)

        self._compatibility_builder = get_compatibility_builder(self._data)

        # Now that we have setup all builders in the director we want to also allow for manual setting
        # of some components. In order to get the desired results we need to perform setter forwarding, e.g
        # self._scale_builder has the setter set_width, then the director should have a method called
        # set_scale_width whose input is forwarded to the actual builder. We can only set this retroactively
        # via monkey-patching.
        self._set_up_setter_forwarding()

    def _set_up_setter_forwarding(self):
        set_up_setter_forwarding_from_director_to_builder(self, "_state_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_mask_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_move_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_reduction_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_slice_event_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_wavelength_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_save_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_scale_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_adjustment_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_normalize_to_monitor_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_calculate_transmission_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_wavelength_and_pixel_adjustment_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_convert_to_q_builder")

        set_up_setter_forwarding_from_director_to_builder(self, "_compatibility_builder")

    def set_user_file(self, user_file):
        file_path = find_full_file_path(user_file)
        if file_path is None:
            raise RuntimeError("UserFileStateDirector: The specified user file cannot be found. Make sure that the "
                               "directory which contains the user file is added to the Mantid path.")
        self._user_file = file_path
        reader = UserFileReader(self._user_file)
        user_file_items = reader.read_user_file()
        self.add_state_settings(user_file_items)

    def add_state_settings(self, user_file_items):
        """
        This allows for a usage of the UserFileStateDirector with externally provided user_file_items or internally
        via the set_user_file method.

        :param user_file_items: a list of parsed user file items.
        """
        # ----------------------------------------------------
        # Populate the different sub states from the user file
        # ----------------------------------------------------
        # Data state
        self._add_information_to_data_state(user_file_items)

        # Mask state
        self._set_up_mask_state(user_file_items)

        # Reduction state
        self._set_up_reduction_state(user_file_items)

        # Move state
        self._set_up_move_state(user_file_items)

        # Wavelength state
        self._set_up_wavelength_state(user_file_items)

        # Slice event state
        self._set_up_slice_event_state(user_file_items)
        # There does not seem to be a command for this currently -- this should be added in the future

        # Scale state
        self._set_up_scale_state(user_file_items)

        # Adjustment state and its substates
        self._set_up_adjustment_state(user_file_items)
        self._set_up_normalize_to_monitor_state(user_file_items)
        self._set_up_calculate_transmission(user_file_items)
        self._set_up_wavelength_and_pixel_adjustment(user_file_items)

        # Convert to Q state
        self._set_up_convert_to_q_state(user_file_items)

        # Compatibility state
        self._set_up_compatibility(user_file_items)

        # Save state
        self._set_up_save(user_file_items)

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

        # Scale state
        scale_state = self._scale_builder.build()
        scale_state.validate()
        self._state_builder.set_scale(scale_state)

        # Adjustment state with the sub states
        normalize_to_monitor_state = self._normalize_to_monitor_builder.build()
        self._adjustment_builder.set_normalize_to_monitor(normalize_to_monitor_state)

        calculate_transmission_state = self._calculate_transmission_builder.build()
        self._adjustment_builder.set_calculate_transmission(calculate_transmission_state)

        wavelength_and_pixel_adjustment_state = self._wavelength_and_pixel_adjustment_builder.build()
        self._adjustment_builder.set_wavelength_and_pixel_adjustment(wavelength_and_pixel_adjustment_state)

        adjustment_state = self._adjustment_builder.build()
        adjustment_state.validate()

        self._state_builder.set_adjustment(adjustment_state)

        # Convert to Q state
        convert_to_q_state = self._convert_to_q_builder.build()
        convert_to_q_state.validate()
        self._state_builder.set_convert_to_q(convert_to_q_state)

        # Compatibility state
        compatibility_state = self._compatibility_builder.build()
        compatibility_state.validate()
        self._state_builder.set_compatibility(compatibility_state)

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
        if DetectorId.correction_x in user_file_items:
            corrections_in_x = user_file_items[DetectorId.correction_x]
            for correction_x in corrections_in_x:
                if correction_x.detector_type is DetectorType.HAB:
                    self._move_builder.set_HAB_x_translation_correction(convert_mm_to_m(correction_x.entry))
                elif correction_x.detector_type is DetectorType.LAB:
                    self._move_builder.set_LAB_x_translation_correction(convert_mm_to_m(correction_x.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " x correction.".format(correction_x.detector_type))

        if DetectorId.correction_y in user_file_items:
            corrections_in_y = user_file_items[DetectorId.correction_y]
            for correction_y in corrections_in_y:
                if correction_y.detector_type is DetectorType.HAB:
                    self._move_builder.set_HAB_y_translation_correction(convert_mm_to_m(correction_y.entry))
                elif correction_y.detector_type is DetectorType.LAB:
                    self._move_builder.set_LAB_y_translation_correction(convert_mm_to_m(correction_y.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " y correction.".format(correction_y.detector_type))

        if DetectorId.correction_z in user_file_items:
            corrections_in_z = user_file_items[DetectorId.correction_z]
            for correction_z in corrections_in_z:
                if correction_z.detector_type is DetectorType.HAB:
                    self._move_builder.set_HAB_z_translation_correction(convert_mm_to_m(correction_z.entry))
                elif correction_z.detector_type is DetectorType.LAB:
                    self._move_builder.set_LAB_z_translation_correction(convert_mm_to_m(correction_z.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " z correction.".format(correction_z.detector_type))

        # ---------------------------
        # Correction for Rotation
        # ---------------------------
        if DetectorId.correction_rotation in user_file_items:
            rotation_correction = user_file_items[DetectorId.correction_rotation]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(rotation_correction, DetectorId.correction_rotation)
            rotation_correction = rotation_correction[-1]
            if rotation_correction.detector_type is DetectorType.HAB:
                self._move_builder.set_HAB_rotation_correction(rotation_correction.entry)
            elif rotation_correction.detector_type is DetectorType.LAB:
                self._move_builder.set_LAB_rotation_correction(rotation_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " rotation correction.".format(rotation_correction.detector_type))

        # ---------------------------
        # Correction for Radius
        # ---------------------------
        if DetectorId.correction_radius in user_file_items:
            radius_corrections = user_file_items[DetectorId.correction_radius]
            for radius_correction in radius_corrections:
                if radius_correction.detector_type is DetectorType.HAB:
                    self._move_builder.set_HAB_radius_correction(convert_mm_to_m(radius_correction.entry))
                elif radius_correction.detector_type is DetectorType.LAB:
                    self._move_builder.set_LAB_radius_correction(convert_mm_to_m(radius_correction.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " radius correction.".format(radius_correction.detector_type))

        # ---------------------------
        # Correction for Translation
        # ---------------------------
        if DetectorId.correction_translation in user_file_items:
            side_corrections = user_file_items[DetectorId.correction_translation]
            for side_correction in side_corrections:
                if side_correction.detector_type is DetectorType.HAB:
                    self._move_builder.set_HAB_side_correction(convert_mm_to_m(side_correction.entry))
                elif side_correction.detector_type is DetectorType.LAB:
                    self._move_builder.set_LAB_side_correction(convert_mm_to_m(side_correction.entry))
                else:
                    raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                       " side correction.".format(side_correction.detector_type))

        # ---------------------------
        # Tilt
        # ---------------------------
        if DetectorId.correction_x_tilt in user_file_items:
            tilt_correction = user_file_items[DetectorId.correction_x_tilt]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.HAB:
                self._move_builder.set_HAB_x_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.LAB:
                self._move_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " titlt correction.".format(tilt_correction.detector_type))

        if DetectorId.correction_y_tilt in user_file_items:
            tilt_correction = user_file_items[DetectorId.correction_y_tilt]
            tilt_correction = tilt_correction[-1]
            if tilt_correction.detector_type is DetectorType.HAB:
                self._move_builder.set_HAB_y_tilt_correction(tilt_correction.entry)
            elif tilt_correction.detector_type is DetectorType.LAB:
                self._move_builder.set_LAB_side_correction(tilt_correction.entry)
            else:
                raise RuntimeError("UserFileStateDirector: An unknown detector {0} was used for the"
                                   " tilt correction.".format(tilt_correction.detector_type))

        # ---------------------------
        # Sample offset
        # ---------------------------
        set_single_entry(self._move_builder, "set_sample_offset", SampleId.offset,
                         user_file_items, apply_to_value=convert_mm_to_m)

        # ---------------------------
        # Monitor 4 offset; for now this is only SANS2D
        # ---------------------------
        if TransId.spec_shift in user_file_items:
            monitor_n_shift = user_file_items[TransId.spec_shift]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(monitor_n_shift, TransId.spec_shift)
            monitor_n_shift = monitor_n_shift[-1]
            set_monitor_n_offset = getattr(self._move_builder, "set_monitor_n_offset", None)
            if isinstance(set_monitor_n_offset, collections.Callable):
                self._move_builder.set_monitor_n_offset(convert_mm_to_m(monitor_n_shift))
            else:
                log_non_existing_field("set_monitor_n_offset")

        # ---------------------------
        # Beam Centre, this can be for HAB and LAB
        # ---------------------------
        if SetId.centre in user_file_items:
            beam_centres = user_file_items[SetId.centre]
            beam_centres_for_hab = [beam_centre for beam_centre in beam_centres if beam_centre.detector_type
                                    is DetectorType.HAB]
            beam_centres_for_lab = [beam_centre for beam_centre in beam_centres if beam_centre.detector_type
                                    is DetectorType.LAB]
            for beam_centre in beam_centres_for_lab:
                pos1 = beam_centre.pos1
                pos2 = beam_centre.pos2
                self._move_builder.set_LAB_sample_centre_pos1(self._move_builder.convert_pos1(pos1))
                self._move_builder.set_LAB_sample_centre_pos2(self._move_builder.convert_pos2(pos2))
                if hasattr(self._move_builder, "set_HAB_sample_centre_pos1"):
                    self._move_builder.set_HAB_sample_centre_pos1(self._move_builder.convert_pos1(pos1))
                if hasattr(self._move_builder, "set_HAB_sample_centre_pos2"):
                    self._move_builder.set_HAB_sample_centre_pos2(self._move_builder.convert_pos2(pos2))

            for beam_centre in beam_centres_for_hab:
                pos1 = beam_centre.pos1
                pos2 = beam_centre.pos2
                self._move_builder.set_HAB_sample_centre_pos1(self._move_builder.convert_pos1(pos1))
                self._move_builder.set_HAB_sample_centre_pos2(self._move_builder.convert_pos2(pos2))

    def _set_up_reduction_state(self, user_file_items):
        # There are several things that can be extracted from the user file
        # 1. The reduction mode
        # 2. The merge behaviour
        # 3. The dimensionality is not set via the user file

        # ------------------------
        # Reduction mode
        # ------------------------
        set_single_entry(self._reduction_builder, "set_reduction_mode", DetectorId.reduction_mode, user_file_items)

        # -------------------------------
        # Shift and rescale
        # -------------------------------
        set_single_entry(self._reduction_builder, "set_merge_scale", DetectorId.rescale, user_file_items)
        set_single_entry(self._reduction_builder, "set_merge_shift", DetectorId.shift, user_file_items)

        # -------------------------------
        # User masking
        # -------------------------------
        merge_min = None
        merge_max = None
        merge_mask = False
        if DetectorId.merge_range in user_file_items:
            merge_range = user_file_items[DetectorId.merge_range]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(merge_range, DetectorId.rescale_fit)
            merge_range = merge_range[-1]
            merge_min = merge_range.start
            merge_max = merge_range.stop
            merge_mask = merge_range.use_fit

        self._reduction_builder.set_merge_mask(merge_mask)
        self._reduction_builder.set_merge_min(merge_min)
        self._reduction_builder.set_merge_max(merge_max)

        # -------------------------------
        # Fitting merged
        # -------------------------------
        q_range_min_scale = None
        q_range_max_scale = None
        has_rescale_fit = False
        if DetectorId.rescale_fit in user_file_items:
            rescale_fits = user_file_items[DetectorId.rescale_fit]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(rescale_fits, DetectorId.rescale_fit)
            rescale_fit = rescale_fits[-1]
            q_range_min_scale = rescale_fit.start
            q_range_max_scale = rescale_fit.stop
            has_rescale_fit = rescale_fit.use_fit

        q_range_min_shift = None
        q_range_max_shift = None
        has_shift_fit = False
        if DetectorId.shift_fit in user_file_items:
            shift_fits = user_file_items[DetectorId.shift_fit]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(shift_fits, DetectorId.shift_fit)
            shift_fit = shift_fits[-1]
            q_range_min_shift = shift_fit.start
            q_range_max_shift = shift_fit.stop
            has_shift_fit = shift_fit.use_fit

        if has_rescale_fit and has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.Both)
            min_q = get_min_q_boundary(q_range_min_scale, q_range_min_shift)
            max_q = get_max_q_boundary(q_range_max_scale, q_range_max_shift)
            if min_q:
                self._reduction_builder.set_merge_range_min(min_q)
            if max_q:
                self._reduction_builder.set_merge_range_max(max_q)
        elif has_rescale_fit and not has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.ScaleOnly)
            if q_range_min_scale:
                self._reduction_builder.set_merge_range_min(q_range_min_scale)
            if q_range_max_scale:
                self._reduction_builder.set_merge_range_max(q_range_max_scale)
        elif not has_rescale_fit and has_shift_fit:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.ShiftOnly)
            if q_range_min_shift:
                self._reduction_builder.set_merge_range_min(q_range_min_shift)
            if q_range_max_shift:
                self._reduction_builder.set_merge_range_max(q_range_max_shift)
        else:
            self._reduction_builder.set_merge_fit_mode(FitModeForMerge.NoFit)

        # ------------------------
        # Reduction Dimensionality
        # ------------------------
        set_single_entry(self._reduction_builder, "set_reduction_dimensionality", OtherId.reduction_dimensionality,
                         user_file_items)

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
        if MaskId.line in user_file_items:
            mask_lines = user_file_items[MaskId.line]
            # If there were several arms specified then we take only the last
            check_if_contains_only_one_element(mask_lines, MaskId.line)
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
        if MaskId.time in user_file_items:
            mask_time_general = user_file_items[MaskId.time]
            start_time = []
            stop_time = []
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
        if MaskId.time_detector in user_file_items:
            mask_times = user_file_items[MaskId.time_detector]
            start_times_hab = []
            stop_times_hab = []
            start_times_lab = []
            stop_times_lab = []
            for times in mask_times:
                if times.start > times.start:
                    raise RuntimeError("UserFileStateDirector: You specified a general time mask with a start time {0}"
                                       " which is larger than the stop time {1} of the mask. This is not"
                                       " valid.".format(times.start, times.stop))
                if times.detector_type is DetectorType.HAB:
                    start_times_hab.append(times.start)
                    stop_times_hab.append(times.stop)
                elif times.detector_type is DetectorType.LAB:
                    start_times_lab.append(times.start)
                    stop_times_lab.append(times.stop)
                else:
                    RuntimeError("UserFileStateDirector: The specified detector {0} is not "
                                 "known".format(times.detector_type))
            if start_times_hab:
                self._mask_builder.set_HAB_bin_mask_start(start_times_hab)
            if stop_times_hab:
                self._mask_builder.set_HAB_bin_mask_stop(stop_times_hab)
            if start_times_lab:
                self._mask_builder.set_LAB_bin_mask_start(start_times_lab)
            if stop_times_lab:
                self._mask_builder.set_LAB_bin_mask_stop(stop_times_lab)

        # ---------------------------------
        # 4. Clear detector
        # ---------------------------------
        if MaskId.clear_detector_mask in user_file_items:
            clear_detector_mask = user_file_items[MaskId.clear_detector_mask]
            check_if_contains_only_one_element(clear_detector_mask, MaskId.clear_detector_mask)
            # We select the entry which was added last.
            clear_detector_mask = clear_detector_mask[-1]
            self._mask_builder.set_clear(clear_detector_mask)

        # ---------------------------------
        # 5. Clear time
        # ---------------------------------
        if MaskId.clear_time_mask in user_file_items:
            clear_time_mask = user_file_items[MaskId.clear_time_mask]
            check_if_contains_only_one_element(clear_time_mask, MaskId.clear_time_mask)
            # We select the entry which was added last.
            clear_time_mask = clear_time_mask[-1]
            self._mask_builder.set_clear_time(clear_time_mask)

        # ---------------------------------
        # 6. Single Spectrum
        # ---------------------------------
        if MaskId.single_spectrum_mask in user_file_items:
            single_spectra = user_file_items[MaskId.single_spectrum_mask]
            # Note that we are using an unusual setter here. Check mask.py for why we are doing this.
            self._mask_builder.set_single_spectra_on_detector(single_spectra)

        # ---------------------------------
        # 7. Spectrum Range
        # ---------------------------------
        if MaskId.spectrum_range_mask in user_file_items:
            spectrum_ranges = user_file_items[MaskId.spectrum_range_mask]
            start_range = []
            stop_range = []
            for spectrum_range in spectrum_ranges:
                if spectrum_range.start > spectrum_range.start:
                    raise RuntimeError("UserFileStateDirector: You specified a spectrum range with a start value {0}"
                                       " which is larger than the stop value {1}. This is not"
                                       " valid.".format(spectrum_range.start, spectrum_range.stop))
                start_range.append(spectrum_range.start)
                stop_range.append(spectrum_range.stop)
            # Note that we are using an unusual setter here. Check mask.py for why we are doing this.
            self._mask_builder.set_spectrum_range_on_detector(start_range, stop_range)

        # ---------------------------------
        # 8. Vertical single strip
        # ---------------------------------
        if MaskId.vertical_single_strip_mask in user_file_items:
            single_vertical_strip_masks = user_file_items[MaskId.vertical_single_strip_mask]
            entry_hab = []
            entry_lab = []
            for single_vertical_strip_mask in single_vertical_strip_masks:
                if single_vertical_strip_mask.detector_type is DetectorType.HAB:
                    entry_hab.append(single_vertical_strip_mask.entry)
                elif single_vertical_strip_mask.detector_type is DetectorType.LAB:
                    entry_lab.append(single_vertical_strip_mask.entry)
                else:
                    raise RuntimeError("UserFileStateDirector: The vertical single strip mask {0} has an unknown "
                                       "detector {1} associated"
                                       " with it.".format(single_vertical_strip_mask.entry,
                                                          single_vertical_strip_mask.detector_type))
            if entry_hab:
                self._mask_builder.set_HAB_single_vertical_strip_mask(entry_hab)
            if entry_lab:
                self._mask_builder.set_LAB_single_vertical_strip_mask(entry_lab)

        # ---------------------------------
        # 9. Vertical range strip
        # ---------------------------------
        if MaskId.vertical_range_strip_mask in user_file_items:
            range_vertical_strip_masks = user_file_items[MaskId.vertical_range_strip_mask]
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
                    raise RuntimeError("UserFileStateDirector: The vertical range strip mask {0} has an unknown "
                                       "detector {1} associated "
                                       "with it.".format(range_vertical_strip_mask.entry,
                                                         range_vertical_strip_mask.detector_type))
            if start_hab:
                self._mask_builder.set_HAB_range_vertical_strip_start(start_hab)
            if stop_hab:
                self._mask_builder.set_HAB_range_vertical_strip_stop(stop_hab)
            if start_lab:
                self._mask_builder.set_LAB_range_vertical_strip_start(start_lab)
            if stop_lab:
                self._mask_builder.set_LAB_range_vertical_strip_stop(stop_lab)

        # ---------------------------------
        # 10. Horizontal single strip
        # ---------------------------------
        if MaskId.horizontal_single_strip_mask in user_file_items:
            single_horizontal_strip_masks = user_file_items[MaskId.horizontal_single_strip_mask]
            entry_hab = []
            entry_lab = []
            for single_horizontal_strip_mask in single_horizontal_strip_masks:
                if single_horizontal_strip_mask.detector_type is DetectorType.HAB:
                    entry_hab.append(single_horizontal_strip_mask.entry)
                elif single_horizontal_strip_mask.detector_type is DetectorType.LAB:
                    entry_lab.append(single_horizontal_strip_mask.entry)
                else:
                    raise RuntimeError("UserFileStateDirector: The horizontal single strip mask {0} has an unknown "
                                       "detector {1} associated"
                                       " with it.".format(single_horizontal_strip_mask.entry,
                                                          single_horizontal_strip_mask.detector_type))
            if entry_hab:
                self._mask_builder.set_HAB_single_horizontal_strip_mask(entry_hab)
            if entry_lab:
                self._mask_builder.set_LAB_single_horizontal_strip_mask(entry_lab)

        # ---------------------------------
        # 11. Horizontal range strip
        # ---------------------------------
        if MaskId.horizontal_range_strip_mask in user_file_items:
            range_horizontal_strip_masks = user_file_items[MaskId.horizontal_range_strip_mask]
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
                    raise RuntimeError("UserFileStateDirector: The vertical range strip mask {0} has an unknown "
                                       "detector {1} associated "
                                       "with it.".format(range_horizontal_strip_mask.entry,
                                                         range_horizontal_strip_mask.detector_type))
            if start_hab:
                self._mask_builder.set_HAB_range_horizontal_strip_start(start_hab)
            if stop_hab:
                self._mask_builder.set_HAB_range_horizontal_strip_stop(stop_hab)
            if start_lab:
                self._mask_builder.set_LAB_range_horizontal_strip_start(start_lab)
            if stop_lab:
                self._mask_builder.set_LAB_range_horizontal_strip_stop(stop_lab)

        # ---------------------------------
        # 12. Block
        # ---------------------------------
        if MaskId.block in user_file_items:
            blocks = user_file_items[MaskId.block]
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
                    raise RuntimeError("UserFileStateDirector: The block mask has an unknown "
                                       "detector {0} associated "
                                       "with it.".format(block.detector_type))
            if horizontal_start_hab:
                self._mask_builder.set_HAB_block_horizontal_start(horizontal_start_hab)
            if horizontal_stop_hab:
                self._mask_builder.set_HAB_block_horizontal_stop(horizontal_stop_hab)
            if vertical_start_lab:
                self._mask_builder.set_LAB_block_vertical_start(vertical_start_lab)
            if vertical_stop_lab:
                self._mask_builder.set_LAB_block_vertical_stop(vertical_stop_lab)

        # ---------------------------------
        # 13. Block cross
        # ---------------------------------
        if MaskId.block_cross in user_file_items:
            block_crosses = user_file_items[MaskId.block_cross]
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
                    raise RuntimeError("UserFileStateDirector: The block cross mask has an unknown "
                                       "detector {0} associated "
                                       "with it.".format(block_cross.detector_type))
            if horizontal_hab:
                self._mask_builder.set_HAB_block_cross_horizontal(horizontal_hab)
            if vertical_hab:
                self._mask_builder.set_HAB_block_cross_vertical(vertical_hab)
            if horizontal_lab:
                self._mask_builder.set_LAB_block_cross_horizontal(horizontal_lab)
            if vertical_lab:
                self._mask_builder.set_LAB_block_cross_vertical(vertical_lab)

        # ------------------------------------------------------------
        # 14. Angles --> they are specified in L/Phi
        # -----------------------------------------------------------
        if LimitsId.angle in user_file_items:
            angles = user_file_items[LimitsId.angle]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(angles, LimitsId.angle)
            angle = angles[-1]
            self._mask_builder.set_phi_min(angle.min)
            self._mask_builder.set_phi_max(angle.max)
            self._mask_builder.set_use_mask_phi_mirror(angle.use_mirror)

        # ------------------------------------------------------------
        # 15. Maskfiles
        # -----------------------------------------------------------
        if MaskId.file in user_file_items:
            mask_files = user_file_items[MaskId.file]
            self._mask_builder.set_mask_files(mask_files)

        # ------------------------------------------------------------
        # 16. Radius masks
        # -----------------------------------------------------------
        if LimitsId.radius in user_file_items:
            radii = user_file_items[LimitsId.radius]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(radii, LimitsId.radius)
            radius = radii[-1]
            if radius.start > radius.stop > 0:
                raise RuntimeError("UserFileStateDirector: The inner radius {0} appears to be larger that the outer"
                                   " radius {1} of the mask.".format(radius.start, radius.stop))
            min_value = None if radius.start is None else convert_mm_to_m(radius.start)
            max_value = None if radius.stop is None else convert_mm_to_m(radius.stop)
            self._mask_builder.set_radius_min(min_value)
            self._mask_builder.set_radius_max(max_value)

    def _set_up_wavelength_state(self, user_file_items):
        set_wavelength_limits(self._wavelength_builder, user_file_items)

    def _set_up_slice_event_state(self, user_file_items):
        # Setting up the slice limits is current
        if OtherId.event_slices in user_file_items:
            event_slices = user_file_items[OtherId.event_slices]
            check_if_contains_only_one_element(event_slices, OtherId.event_slices)
            event_slices = event_slices[-1]
            # The events binning can come in three forms.
            # 1. As a simple range object
            # 2. As an already parsed rebin array, ie min, step, max
            # 3. As a string. Note that this includes custom commands.
            if isinstance(event_slices, simple_range):
                start, stop = get_ranges_for_rebin_setting(event_slices.start, event_slices.stop,
                                                           event_slices.step, event_slices.step_type)
            elif isinstance(event_slices, rebin_string_values):
                start, stop = get_ranges_for_rebin_array(event_slices.value)
            else:
                start, stop = get_ranges_from_event_slice_setting(event_slices.value)
            self._slice_event_builder.set_start_time(start)
            self._slice_event_builder.set_end_time(stop)

    def _set_up_scale_state(self, user_file_items):
        # We only extract the first entry here, ie the s entry. Although there are other entries which a user can
        # specify such as a, b, c, d they seem to be
        if SetId.scales in user_file_items:
            scales = user_file_items[SetId.scales]
            check_if_contains_only_one_element(scales, SetId.scales)
            scales = scales[-1]
            self._scale_builder.set_scale(scales.s)

        # We can also have settings for the sample geometry (Note that at the moment this is not settable via the
        # user file nor the command line interface
        if OtherId.sample_shape in user_file_items:
            sample_shape = user_file_items[OtherId.sample_shape]
            check_if_contains_only_one_element(sample_shape, OtherId.sample_shape)
            sample_shape = sample_shape[-1]
            self._scale_builder.set_shape(sample_shape)

        if OtherId.sample_width in user_file_items:
            sample_width = user_file_items[OtherId.sample_width]
            check_if_contains_only_one_element(sample_width, OtherId.sample_width)
            sample_width = sample_width[-1]
            self._scale_builder.set_width(sample_width)

        if OtherId.sample_height in user_file_items:
            sample_height = user_file_items[OtherId.sample_height]
            check_if_contains_only_one_element(sample_height, OtherId.sample_height)
            sample_height = sample_height[-1]
            self._scale_builder.set_height(sample_height)

        if OtherId.sample_thickness in user_file_items:
            sample_thickness = user_file_items[OtherId.sample_thickness]
            check_if_contains_only_one_element(sample_thickness, OtherId.sample_thickness)
            sample_thickness = sample_thickness[-1]
            self._scale_builder.set_thickness(sample_thickness)

    def _set_up_convert_to_q_state(self, user_file_items):
        # Get the radius cut off if any is present
        set_single_entry(self._convert_to_q_builder, "set_radius_cutoff", LimitsId.radius_cut, user_file_items,
                         apply_to_value=convert_mm_to_m)

        # Get the wavelength cut off if any is present
        set_single_entry(self._convert_to_q_builder, "set_wavelength_cutoff", LimitsId.wavelength_cut,
                         user_file_items)

        # Get the 1D q values
        if LimitsId.q in user_file_items:
            limits_q = user_file_items[LimitsId.q]
            check_if_contains_only_one_element(limits_q, LimitsId.q)
            limits_q = limits_q[-1]
            self._convert_to_q_builder.set_q_min(limits_q.min)
            self._convert_to_q_builder.set_q_max(limits_q.max)
            self._convert_to_q_builder.set_q_1d_rebin_string(limits_q.rebin_string)

        # Get the 2D q values
        if LimitsId.qxy in user_file_items:
            limits_qxy = user_file_items[LimitsId.qxy]
            check_if_contains_only_one_element(limits_qxy, LimitsId.qxy)
            limits_qxy = limits_qxy[-1]
            # Now we have to check if we have a simple pattern or a more complex pattern at hand
            is_complex = isinstance(limits_qxy, complex_range)
            self._convert_to_q_builder.set_q_xy_max(limits_qxy.stop)
            if is_complex:
                # Note that it has not been implemented in the old reducer, but the documentation is
                #  suggesting that it is available. Hence we throw here.
                raise RuntimeError("Qxy cannot handle settings of type: L/Q l1,dl1,l3,dl2,l2 [/LIN|/LOG] ")
            else:
                self._convert_to_q_builder.set_q_xy_step(limits_qxy.step)
                self._convert_to_q_builder.set_q_xy_step_type(limits_qxy.step_type)

        # Get the Gravity settings
        set_single_entry(self._convert_to_q_builder, "set_use_gravity", GravityId.on_off, user_file_items)
        set_single_entry(self._convert_to_q_builder, "set_gravity_extra_length", GravityId.extra_length,
                         user_file_items)

        # Get the QResolution settings set_q_resolution_delta_r
        set_single_entry(self._convert_to_q_builder, "set_use_q_resolution", QResolutionId.on, user_file_items)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_delta_r", QResolutionId.delta_r,
                         user_file_items, apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_collimation_length",
                         QResolutionId.collimation_length, user_file_items)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_a1", QResolutionId.a1, user_file_items,
                         apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_a2", QResolutionId.a2, user_file_items,
                         apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_moderator_file", QResolutionId.moderator,
                         user_file_items)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_h1", QResolutionId.h1, user_file_items,
                         apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_h2", QResolutionId.h2, user_file_items,
                         apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_w1", QResolutionId.w1, user_file_items,
                         apply_to_value=convert_mm_to_m)
        set_single_entry(self._convert_to_q_builder, "set_q_resolution_w2", QResolutionId.w2, user_file_items,
                         apply_to_value=convert_mm_to_m)

        # ------------------------
        # Reduction Dimensionality
        # ------------------------
        set_single_entry(self._convert_to_q_builder, "set_reduction_dimensionality", OtherId.reduction_dimensionality,
                         user_file_items)

    def _set_up_adjustment_state(self, user_file_items):
        # Get the wide angle correction setting
        set_single_entry(self._adjustment_builder, "set_wide_angle_correction", SampleId.path, user_file_items)

    def _set_up_normalize_to_monitor_state(self, user_file_items):
        # Extract the incident monitor and which type of rebinning to use (interpolating or normal)
        if MonId.spectrum in user_file_items:
            mon_spectrum = user_file_items[MonId.spectrum]
            mon_spec = [spec for spec in mon_spectrum if not spec.is_trans]

            if mon_spec:
                mon_spec = mon_spec[-1]
                rebin_type = RebinType.InterpolatingRebin if mon_spec.interpolate else RebinType.Rebin
                self._normalize_to_monitor_builder.set_rebin_type(rebin_type)

                #  We have to check if the spectrum is None, this can be the case when the user wants to use the
                # default incident monitor spectrum
                if mon_spec.spectrum:
                    self._normalize_to_monitor_builder.set_incident_monitor(mon_spec.spectrum)

        # The prompt peak correction values
        set_prompt_peak_correction(self._normalize_to_monitor_builder, user_file_items)

        # The general background settings
        set_background_tof_general(self._normalize_to_monitor_builder, user_file_items)

        # The monitor-specific background settings
        set_background_tof_monitor(self._normalize_to_monitor_builder, user_file_items)

        # Get the wavelength rebin settings
        set_wavelength_limits(self._normalize_to_monitor_builder, user_file_items)

    def _set_up_calculate_transmission(self, user_file_items):
        # Transmission radius
        set_single_entry(self._calculate_transmission_builder, "set_transmission_radius_on_detector", TransId.radius,
                         user_file_items, apply_to_value=convert_mm_to_m)

        # List of transmission roi files
        if TransId.roi in user_file_items:
            trans_roi = user_file_items[TransId.roi]
            self._calculate_transmission_builder.set_transmission_roi_files(trans_roi)

        # List of transmission mask files
        if TransId.mask in user_file_items:
            trans_mask = user_file_items[TransId.mask]
            self._calculate_transmission_builder.set_transmission_mask_files(trans_mask)

        # The prompt peak correction values
        set_prompt_peak_correction(self._calculate_transmission_builder, user_file_items)

        # The transmission spectrum
        if TransId.spec in user_file_items:
            trans_spec = user_file_items[TransId.spec]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(trans_spec, TransId.spec)
            trans_spec = trans_spec[-1]
            self._calculate_transmission_builder.set_transmission_monitor(trans_spec)

        # The incident monitor spectrum for transmission calculation
        if MonId.spectrum in user_file_items:
            mon_spectrum = user_file_items[MonId.spectrum]
            mon_spec = [spec for spec in mon_spectrum if spec.is_trans]
            if mon_spec:
                mon_spec = mon_spec[-1]
                rebin_type = RebinType.InterpolatingRebin if mon_spec.interpolate else RebinType.Rebin
                self._calculate_transmission_builder.set_rebin_type(rebin_type)

                # We have to check if the spectrum is None, this can be the case when the user wants to use the
                # default incident monitor spectrum
                if mon_spec.spectrum:
                    self._calculate_transmission_builder.set_incident_monitor(mon_spec.spectrum)

        # The general background settings
        set_background_tof_general(self._calculate_transmission_builder, user_file_items)

        # The monitor-specific background settings
        set_background_tof_monitor(self._calculate_transmission_builder, user_file_items)

        # The roi-specific background settings
        if BackId.trans in user_file_items:
            back_trans = user_file_items[BackId.trans]
            # Should the user have chosen several values, then the last element is selected
            check_if_contains_only_one_element(back_trans, BackId.trans)
            back_trans = back_trans[-1]
            self._calculate_transmission_builder.set_background_TOF_roi_start(back_trans.start)
            self._calculate_transmission_builder.set_background_TOF_roi_stop(back_trans.stop)

        # Set the fit settings
        if FitId.general in user_file_items:
            fit_general = user_file_items[FitId.general]
            # We can have settings for both the sample or the can or individually
            # There can be three types of settings:
            # 1. Clearing the fit setting
            # 2. General settings where the entry data_type is not specified. Settings apply to both sample and can
            # 3. Sample settings
            # 4. Can settings
            # We first apply the general settings. Specialized settings for can or sample override the general settings
            # As usual if there are multiple settings for a specific case, then the last in the list is used.

            # 1 Fit type settings
            clear_settings = [item for item in fit_general if item.data_type is None and item.fit_type is FitType.NoFit]

            if clear_settings:
                check_if_contains_only_one_element(clear_settings, FitId.general)
                clear_settings = clear_settings[-1]
                # Will set the fitting to NoFit
                self._calculate_transmission_builder.set_Sample_fit_type(clear_settings.fit_type)
                self._calculate_transmission_builder.set_Can_fit_type(clear_settings.fit_type)

            # 2. General settings
            general_settings = [item for item in fit_general if item.data_type is None and
                                item.fit_type is not FitType.NoFit]
            if general_settings:
                check_if_contains_only_one_element(general_settings, FitId.general)
                general_settings = general_settings[-1]
                self._calculate_transmission_builder.set_Sample_fit_type(general_settings.fit_type)
                self._calculate_transmission_builder.set_Sample_polynomial_order(general_settings.polynomial_order)
                self._calculate_transmission_builder.set_Sample_wavelength_low(general_settings.start)
                self._calculate_transmission_builder.set_Sample_wavelength_high(general_settings.stop)
                self._calculate_transmission_builder.set_Can_fit_type(general_settings.fit_type)
                self._calculate_transmission_builder.set_Can_polynomial_order(general_settings.polynomial_order)
                self._calculate_transmission_builder.set_Can_wavelength_low(general_settings.start)
                self._calculate_transmission_builder.set_Can_wavelength_high(general_settings.stop)

            # 3. Sample settings
            sample_settings = [item for item in fit_general if item.data_type is DataType.Sample]
            if sample_settings:
                check_if_contains_only_one_element(sample_settings, FitId.general)
                sample_settings = sample_settings[-1]
                self._calculate_transmission_builder.set_Sample_fit_type(sample_settings.fit_type)
                self._calculate_transmission_builder.set_Sample_polynomial_order(sample_settings.polynomial_order)
                self._calculate_transmission_builder.set_Sample_wavelength_low(sample_settings.start)
                self._calculate_transmission_builder.set_Sample_wavelength_high(sample_settings.stop)

            # 4. Can settings
            can_settings = [item for item in fit_general if item.data_type is DataType.Can]
            if can_settings:
                check_if_contains_only_one_element(can_settings, FitId.general)
                can_settings = can_settings[-1]
                self._calculate_transmission_builder.set_Can_fit_type(can_settings.fit_type)
                self._calculate_transmission_builder.set_Can_polynomial_order(can_settings.polynomial_order)
                self._calculate_transmission_builder.set_Can_wavelength_low(can_settings.start)
                self._calculate_transmission_builder.set_Can_wavelength_high(can_settings.stop)

        # Set the wavelength default configuration
        set_wavelength_limits(self._calculate_transmission_builder, user_file_items)

        # Set the full wavelength range. Note that this can currently only be set from the ISISCommandInterface
        if OtherId.use_full_wavelength_range in user_file_items:
            use_full_wavelength_range = user_file_items[OtherId.use_full_wavelength_range]
            check_if_contains_only_one_element(use_full_wavelength_range, OtherId.use_full_wavelength_range)
            use_full_wavelength_range = use_full_wavelength_range[-1]
            self._calculate_transmission_builder.set_use_full_wavelength_range(use_full_wavelength_range)

    def _set_up_wavelength_and_pixel_adjustment(self, user_file_items):
        # Get the flat/flood files. There can be entries for LAB and HAB.
        if MonId.flat in user_file_items:
            mon_flat = user_file_items[MonId.flat]
            hab_flat_entries = [item for item in mon_flat if item.detector_type is DetectorType.HAB]
            lab_flat_entries = [item for item in mon_flat if item.detector_type is DetectorType.LAB]
            if hab_flat_entries:
                hab_flat_entry = hab_flat_entries[-1]
                self._wavelength_and_pixel_adjustment_builder.set_HAB_pixel_adjustment_file(hab_flat_entry.file_path)

            if lab_flat_entries:
                lab_flat_entry = lab_flat_entries[-1]
                self._wavelength_and_pixel_adjustment_builder.set_LAB_pixel_adjustment_file(lab_flat_entry.file_path)

        # Get the direct files. There can be entries for LAB and HAB.
        if MonId.direct in user_file_items:
            mon_direct = user_file_items[MonId.direct]
            hab_direct_entries = [item for item in mon_direct if item.detector_type is DetectorType.HAB]
            lab_direct_entries = [item for item in mon_direct if item.detector_type is DetectorType.LAB]
            if hab_direct_entries:
                hab_direct_entry = hab_direct_entries[-1]
                self._wavelength_and_pixel_adjustment_builder.set_HAB_wavelength_adjustment_file(
                    hab_direct_entry.file_path)

            if lab_direct_entries:
                lab_direct_entry = lab_direct_entries[-1]
                self._wavelength_and_pixel_adjustment_builder.set_LAB_wavelength_adjustment_file(
                    lab_direct_entry.file_path)

        # Set up the wavelength
        set_wavelength_limits(self._wavelength_and_pixel_adjustment_builder, user_file_items)

    def _set_up_compatibility(self, user_file_items):
        if LimitsId.events_binning in user_file_items:
            events_binning = user_file_items[LimitsId.events_binning]
            check_if_contains_only_one_element(events_binning, LimitsId.events_binning)
            events_binning = events_binning[-1]
            self._compatibility_builder.set_time_rebin_string(events_binning)

        if OtherId.use_compatibility_mode in user_file_items:
            use_compatibility_mode = user_file_items[OtherId.use_compatibility_mode]
            check_if_contains_only_one_element(use_compatibility_mode, OtherId.use_compatibility_mode)
            use_compatibility_mode = use_compatibility_mode[-1]
            self._compatibility_builder.set_use_compatibility_mode(use_compatibility_mode)

        if OtherId.use_event_slice_optimisation in user_file_items:
            use_event_slice_optimisation = user_file_items[OtherId.use_event_slice_optimisation]
            check_if_contains_only_one_element(use_event_slice_optimisation, OtherId.use_event_slice_optimisation)
            use_event_slice_optimisation = use_event_slice_optimisation[-1]
            self._compatibility_builder.set_use_event_slice_optimisation(use_event_slice_optimisation)

    def _set_up_save(self, user_file_items):
        if OtherId.save_types in user_file_items:
            save_types = user_file_items[OtherId.save_types]
            check_if_contains_only_one_element(save_types, OtherId.save_types)
            save_types = save_types[-1]
            self._save_builder.set_file_format(save_types)

        if OtherId.save_as_zero_error_free in user_file_items:
            save_as_zero_error_free = user_file_items[OtherId.save_as_zero_error_free]
            check_if_contains_only_one_element(save_as_zero_error_free, OtherId.save_as_zero_error_free)
            save_as_zero_error_free = save_as_zero_error_free[-1]
            self._save_builder.set_zero_free_correction(save_as_zero_error_free)

        if OtherId.user_specified_output_name in user_file_items:
            user_specified_output_name = user_file_items[OtherId.user_specified_output_name]
            check_if_contains_only_one_element(user_specified_output_name, OtherId.user_specified_output_name)
            user_specified_output_name = user_specified_output_name[-1]
            self._save_builder.set_user_specified_output_name(user_specified_output_name)

        if OtherId.user_specified_output_name_suffix in user_file_items:
            user_specified_output_name_suffix = user_file_items[OtherId.user_specified_output_name_suffix]
            check_if_contains_only_one_element(user_specified_output_name_suffix,
                                               OtherId.user_specified_output_name_suffix)
            user_specified_output_name_suffix = user_specified_output_name_suffix[-1]
            self._save_builder.set_user_specified_output_name_suffix(user_specified_output_name_suffix)

        if OtherId.use_reduction_mode_as_suffix in user_file_items:
            use_reduction_mode_as_suffix = user_file_items[OtherId.use_reduction_mode_as_suffix]
            check_if_contains_only_one_element(use_reduction_mode_as_suffix,
                                               OtherId.use_reduction_mode_as_suffix)
            use_reduction_mode_as_suffix = use_reduction_mode_as_suffix[-1]
            self._save_builder.set_use_reduction_mode_as_suffix(use_reduction_mode_as_suffix)

    def _add_information_to_data_state(self, user_file_items):
        # The only thing that should be set on the data is the tube calibration file which is specified in
        # the user file.
        if TubeCalibrationFileId.file in user_file_items:
            tube_calibration = user_file_items[TubeCalibrationFileId.file]
            check_if_contains_only_one_element(tube_calibration, TubeCalibrationFileId.file)
            tube_calibration = tube_calibration[-1]
            self._data.calibration = tube_calibration

    def convert_pos1(self, pos1):
        """
        Performs a conversion of position 1 of the beam centre. This is forwarded to the move builder.

        :param pos1: the first position (this can be x in mm or for LARMOR and angle)
        :return: the correctly scaled position
        """
        return self._move_builder.convert_pos1(pos1)

    def convert_pos2(self, pos2):
        """
        Performs a conversion of position 2 of the beam centre. This is forwarded to the move builder.

        :param pos2: the second position
        :return: the correctly scaled position
        """
        return self._move_builder.convert_pos2(pos2)
