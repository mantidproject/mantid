# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import abc

from sans.common.Containers.FloatRange import FloatRange
from sans.common.Containers.MonitorID import MonitorID
from sans.common.Containers.Position import XYPosition
from sans.common.enums import SANSInstrument, CorrectionType, FitType
from sans.user_file.IUserFileParser import IUserFileParser
from sans.user_file.parsed_containers.BackgroundDetails import BackgroundDetails
from sans.user_file.parsed_containers.DetectorDetails import DetectorDetails
from sans.user_file.parsed_containers.FitDetails import FitDetails
from sans.user_file.parsed_containers.LimitDetails import LimitDetails
from sans.user_file.parsed_containers.MaskDetails import MaskDetails
from sans.user_file.parsed_containers.MonitorDetails import MonitorDetails
from sans.user_file.parsed_containers.QResolutionDetails import QResolutionDetails
from sans.user_file.parsed_containers.SampleDetails import SampleDetails
from sans.user_file.parsed_containers.SaveDetails import SaveDetails
from sans.user_file.parsed_containers.SetPositionDetails import SetPositionDetails
from sans.user_file.parsed_containers.TransmissionDetails import TransmissionDetails
from sans.user_file.settings_tags import BackId, DetectorId, FitId, GravityId, LimitsId, MaskId, MonId, QResolutionId, \
    SampleId, SetId, TransId, TubeCalibrationFileId, OtherId


class ParsedDictConverter(IUserFileParser, abc.ABC):
    def __init__(self):
        super(ParsedDictConverter, self).__init__()
        self._cached_result = None

    @property
    def _output(self):
        if not self._cached_result:
            self._cached_result = self._get_input_dict()
            # Ensure we always have a dict
            self._cached_result = self._cached_result if self._cached_result else {}
        return self._cached_result

    @abc.abstractmethod
    def _get_input_dict(self) -> dict:
        """
        Gets the dictionary to translate as an input dictionary
        :return: Dictionary to translate
        """
        pass

    def _get_val(self, key):
        return self._output.get(key)

    @staticmethod
    def _to_monitor_id(val):
        """
        Converts the value to a monitor ID or returns None
        """
        assert (isinstance(val, int) or val is None)
        return MonitorID(monitor_spec_num=val) if val is not None else val

    @staticmethod
    def _to_float_range(val):
        if val:
            return FloatRange(start=val.start, end=val.stop)

    @staticmethod
    def _wrap_in_list(val):
        """
        Wraps the value in a list if it is not none, otherwise returns none
        """
        return [val] if val else val

    @staticmethod
    def _to_bool(val):
        return True if val else False

    def get_background_details(self) -> BackgroundDetails:
        all_monitors = self._to_float_range(self._get_val(BackId.ALL_MONITORS))
        transmission_ids = self._to_float_range(self._get_val(BackId.TRANS))
        monitor_with_bck_off = self._to_monitor_id(self._get_val(BackId.MONITOR_OFF))
        monitor_with_bck_off = self._wrap_in_list(monitor_with_bck_off)

        tof_single_mon = self._get_val(BackId.SINGLE_MONITORS)
        if tof_single_mon:
            monitor_id = MonitorID(monitor_spec_num=tof_single_mon.monitor)
            monitor_tuple = (monitor_id, self._to_float_range(tof_single_mon))
            tof_single_mon = self._wrap_in_list(monitor_tuple)

        return BackgroundDetails(monitors_with_background_off=monitor_with_bck_off,
                                 transmission_tof_range=transmission_ids,
                                 tof_window_all_monitors=all_monitors,
                                 tof_window_single_monitor=tof_single_mon)

    def get_detector_details(self) -> DetectorDetails:
        mapped_pairs = [(DetectorId.CORRECTION_X, CorrectionType.X),
                        (DetectorId.CORRECTION_Y, CorrectionType.Y),
                        (DetectorId.CORRECTION_Z, CorrectionType.Z),
                        (DetectorId.CORRECTION_RADIUS, CorrectionType.RADIUS),
                        (DetectorId.CORRECTION_ROTATION, CorrectionType.ROTATION),
                        (DetectorId.CORRECTION_TRANSLATION, CorrectionType.TRANSLATION),
                        (DetectorId.CORRECTION_X_TILT, CorrectionType.X_TILT),
                        (DetectorId.CORRECTION_Y_TILT, CorrectionType.Y_TILT)]

        adjustment_list = {}

        for old_name, new_name in mapped_pairs:
            if self._get_val(old_name):
                adjustment_list[new_name] = self._get_val(old_name)

        rescale_range = self._to_float_range(self._get_val(DetectorId.RESCALE_FIT))
        shift_range = self._to_float_range(self._get_val(DetectorId.SHIFT_FIT))

        return DetectorDetails(detector_adjustment=adjustment_list,
                               reduction_mode=self._get_val(DetectorId.REDUCTION_MODE),
                               merge_fitted_rescale=rescale_range,
                               merge_fitted_shift=shift_range,
                               merge_rescale=self._get_val(DetectorId.RESCALE),
                               merge_shift=self._get_val(DetectorId.SHIFT))

    def get_fit_details(self) -> FitDetails:
        transmission_fit = self._get_val(FitId.GENERAL)
        transmission_fit_enabled = True if transmission_fit and transmission_fit.fit_type != FitType.NO_FIT \
            else False

        monitor_fit = self._to_float_range(self._get_val(FitId.MONITOR_TIMES))
        monitor_fit_enabled = True if monitor_fit else False

        return FitDetails(transmission_fit=(transmission_fit_enabled, transmission_fit),
                          monitor_times=(monitor_fit_enabled, monitor_fit))

    def get_gravity_details(self) -> (bool, float):
        extra_len = self._get_val(GravityId.EXTRA_LENGTH)
        extra_len = 0 if not extra_len else extra_len

        gravity_on = self._to_bool(self._get_val(GravityId.ON_OFF))

        return gravity_on, extra_len

    def get_instrument(self) -> SANSInstrument:
        selected_inst = self._get_val(DetectorId.INSTRUMENT)
        if not selected_inst or selected_inst == SANSInstrument.NO_INSTRUMENT:
            selected_inst = SANSInstrument.NO_INSTRUMENT

        return selected_inst

    def get_limit_details(self) -> LimitDetails:
        radius_cut_limit = self._get_val(LimitsId.RADIUS_CUT)
        wavelength_cut_limit = self._get_val(LimitsId.WAVELENGTH_CUT)

        radius_range = self._to_float_range(self._get_val(LimitsId.RADIUS))
        cuts = []
        if radius_cut_limit:
            radius_cut_tuple = (LimitsId.RADIUS_CUT, radius_cut_limit)
            cuts.extend(self._wrap_in_list(radius_cut_tuple))

        if wavelength_cut_limit:
            wavelength_cut_tuple = (LimitsId.WAVELENGTH_CUT, wavelength_cut_limit)
            cuts.extend(self._wrap_in_list(wavelength_cut_tuple))

        use_full_wavelength = self._to_bool(self._get_val(OtherId.USE_FULL_WAVELENGTH_RANGE))

        return LimitDetails(angle_limit=self._get_val(LimitsId.ANGLE),
                            event_binning=self._get_val(LimitsId.EVENTS_BINNING),
                            cut_limit=cuts,
                            radius_range=radius_range,
                            q_limits=self._get_val(LimitsId.Q),
                            qxy_limit=self._get_val(LimitsId.QXY),
                            use_full_wavelength=use_full_wavelength,
                            wavelength_limit=self._get_val(LimitsId.WAVELENGTH))

    def get_mask_details(self) -> MaskDetails:
        block_masks = self._wrap_in_list(self._get_val(MaskId.BLOCK))
        block_cross = self._wrap_in_list(self._get_val(MaskId.BLOCK_CROSS))
        line_masks = self._wrap_in_list(self._get_val(MaskId.LINE))
        time_masks = self._wrap_in_list(self._get_val(MaskId.TIME))
        time_det_masks = self._wrap_in_list(self._get_val(MaskId.TIME_DETECTOR))

        clear_detector_mask = self._to_bool(self._get_val(MaskId.CLEAR_DETECTOR_MASK))
        clear_time_mask = self._to_bool(self._get_val(MaskId.CLEAR_TIME_MASK))

        horizontal_strips = []
        vertical_strips = []

        def append_single_to_strip(val, list_to_append):
            if val:
                list_to_append.append((val.detector_type, val.entry))

        append_single_to_strip(self._get_val(MaskId.HORIZONTAL_SINGLE_STRIP_MASK), horizontal_strips)
        append_single_to_strip(self._get_val(MaskId.VERTICAL_SINGLE_STRIP_MASK), vertical_strips)

        def append_multiple_to_strips(val, list_to_append):
            if not val:
                return
            for index in range(val.start, val.stop + 1):
                list_to_append.append((val.detector_type, index))

        append_multiple_to_strips(self._get_val(MaskId.HORIZONTAL_RANGE_STRIP_MASK), horizontal_strips)
        append_multiple_to_strips(self._get_val(MaskId.VERTICAL_RANGE_STRIP_MASK), vertical_strips)

        mask_spectra_nums = []
        mask_spectra_single = self._get_val(MaskId.SINGLE_SPECTRUM_MASK)
        mask_spectra_range = self._get_val(MaskId.SPECTRUM_RANGE_MASK)
        if mask_spectra_single:
            mask_spectra_nums.append(mask_spectra_single)

        if mask_spectra_range:
            for i in range(mask_spectra_range.start, mask_spectra_range.stop + 1):
                mask_spectra_nums.append(int(i))

        return MaskDetails(block_masks=block_masks,
                           block_crosses=block_cross,
                           line_masks=line_masks,
                           time_masks=time_masks,
                           time_masks_with_detector=time_det_masks,
                           remove_detector_masks=clear_detector_mask,
                           remove_time_masks=clear_time_mask,
                           mask_horizontal_strips=horizontal_strips,
                           mask_vertical_strips=vertical_strips,
                           mask_spectra_nums=mask_spectra_nums,
                           mask_filenames=self._get_val(MaskId.FILE))

    def get_monitor_details(self) -> MonitorDetails:
        def unpack_monitor_file(val):
            if val:
                return {val.detector_type: val.file_path}

        direct_filename = unpack_monitor_file(self._get_val(MonId.DIRECT))
        flood_filename = unpack_monitor_file(self._get_val(MonId.FLAT))
        mon_pos = self._wrap_in_list(self._get_val(MonId.LENGTH))

        return MonitorDetails(direct_filename=direct_filename,
                              flood_source_filename=flood_filename,
                              monitor_pos_from_moderator=mon_pos,
                              selected_spectrum=self._get_val(MonId.SPECTRUM))

    def get_q_resolution_details(self) -> QResolutionDetails:
        q_resolution_on = self._to_bool(self._get_val(QResolutionId.ON))

        return QResolutionDetails(use_q_resolution_calculation=q_resolution_on,
                                  a1=self._get_val(QResolutionId.A1),
                                  a2=self._get_val(QResolutionId.A2),
                                  h1=self._get_val(QResolutionId.H1),
                                  h2=self._get_val(QResolutionId.H2),
                                  w1=self._get_val(QResolutionId.W1),
                                  w2=self._get_val(QResolutionId.W2),
                                  collimation_length=self._get_val(QResolutionId.COLLIMATION_LENGTH),
                                  delta_r=self._get_val(QResolutionId.DELTA_R),
                                  moderator_filename=self._get_val(QResolutionId.MODERATOR))

    def get_sample_details(self) -> SampleDetails:
        wide_angle_on = self._to_bool(self._get_val(SampleId.PATH))

        return SampleDetails(event_slices=self._get_val(OtherId.EVENT_SLICES),
                             selected_dimension=self._get_val(OtherId.REDUCTION_DIMENSIONALITY),
                             wide_angle_corrections=wide_angle_on,
                             z_offset=self._get_val(SampleId.OFFSET))

    def get_save_details(self) -> SaveDetails:
        save_as_zero_err_free = self._to_bool(self._get_val(OtherId.SAVE_AS_ZERO_ERROR_FREE))
        use_reduction_mode_as_suffix = self._to_bool(self._get_val(OtherId.USE_REDUCTION_MODE_AS_SUFFIX))
        return SaveDetails(save_as_zero_error_free=save_as_zero_err_free,
                           selected_save_algs=self._get_val(OtherId.SAVE_TYPES),
                           use_reduction_mode_as_suffix=use_reduction_mode_as_suffix,
                           user_output_name=self._get_val(OtherId.USER_SPECIFIED_OUTPUT_NAME),
                           user_output_suffix=self._get_val(OtherId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX))

    def get_set_position_details(self) -> SetPositionDetails:
        def unpack_pos_and_det(val):
            if val:
                return {val.detector_type: XYPosition(X=val.pos1, Y=val.pos2)}
            else:
                return {}

        centre_dict = {}
        centre_dict.update(unpack_pos_and_det(self._get_val(SetId.CENTRE)))
        centre_dict.update(unpack_pos_and_det(self._get_val(SetId.CENTRE_HAB)))

        return SetPositionDetails(centre=centre_dict,
                                  scales=self._get_val(SetId.SCALES))

    def get_transmission_details(self) -> TransmissionDetails:
        def pack_spec_shift(spec_num, val, existing_list):
            if val:
                tuple_to_pack = (MonitorID(monitor_spec_num=spec_num), val)
                existing_list.append(tuple_to_pack)

        shifts = []
        pack_spec_shift(4, self._get_val(TransId.SPEC_4_SHIFT), shifts)
        pack_spec_shift(5, self._get_val(TransId.SPEC_5_SHIFT), shifts)

        return TransmissionDetails(selected_spectrum=self._get_val(TransId.SPEC),
                                   spectrum_shifts=shifts,
                                   selected_radius=self._get_val(TransId.RADIUS),
                                   selected_mask_filenames=self._get_val(TransId.MASK),
                                   selected_roi_filenames=self._get_val(TransId.ROI),
                                   selected_can_workspace=self._get_val(TransId.CAN_WORKSPACE),
                                   selected_sample_workspace=self._get_val(TransId.SAMPLE_WORKSPACE))

    def get_tube_calibration_filename(self) -> str:
        return self._get_val(TubeCalibrationFileId.FILE)

    def is_compatibility_mode_on(self) -> bool:
        return self._to_bool(self._get_val(OtherId.USE_COMPATIBILITY_MODE))
