# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.common.enums import SANSInstrument, ReductionMode, DetectorType, RangeStepType, RebinType, FitModeForMerge, \
    DataType, FitType
from sans.common.general_functions import get_bank_for_spectrum_number
from sans.state.IStateParser import IStateParser
from sans.state.StateObjects.StateAdjustment import StateAdjustment
from sans.state.StateObjects.StateCalculateTransmission import get_calculate_transmission
from sans.state.StateObjects.StateCompatibility import StateCompatibility
from sans.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans.state.StateObjects.StateMaskDetectors import get_mask_builder, StateMaskDetectors
from sans.state.StateObjects.StateMoveDetectors import get_move_builder
from sans.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor, get_normalize_to_monitor_builder
from sans.state.StateObjects.StateReductionMode import StateReductionMode
from sans.state.StateObjects.StateScale import StateScale
from sans.state.StateObjects.StateSave import StateSave
from sans.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans.state.StateObjects.StateWavelength import StateWavelength
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment
from sans.user_file.toml_parsers.toml_v1_schema import TomlSchemaV1Validator


class TomlV1Parser(IStateParser):
    def __init__(self, dict_to_parse, data_info, schema_validator=None):
        self._validator = schema_validator if schema_validator else TomlSchemaV1Validator(dict_to_parse)
        self._validator.validate()

        self._implementation = self._get_impl(dict_to_parse, data_info)
        self._implementation.parse_all()

    @staticmethod
    def _get_impl(*args):
        # Wrapper which can replaced with a mock
        return _TomlV1ParserImpl(*args)

    def get_state_adjustment(self):
        return self._implementation.adjustment

    def get_state_calculate_transmission(self):
        return self._implementation.calculate_transmission

    def get_state_compatibility(self):
        return self._implementation.compatibility

    def get_state_convert_to_q(self):
        return self._implementation.convert_to_q

    def get_state_data(self):
        return self._implementation.data_info

    def get_state_mask(self):
        return self._implementation.mask

    def get_state_move(self):
        return self._implementation.move

    def get_state_normalize_to_monitor(self):
        return self._implementation.normalize_to_monitor

    def get_state_reduction_mode(self):
        return self._implementation.reduction_mode

    def get_state_save(self):
        return StateSave()

    def get_state_scale(self):
        return self._implementation.scale

    def get_state_slice_event(self):
        return StateSliceEvent()

    def get_state_wavelength(self):
        return self._implementation.wavelength

    def get_state_wavelength_and_pixel_adjustment(self):
        return self._implementation.wavelength_and_pixel


class _TomlV1ParserImpl(object):
    # TODO this should not be in the TOML parser so we should unpick it at a later stage
    def __init__(self, input_dict, data_info):
        self._input = input_dict
        self.data_info = data_info
        self._create_state_objs()

    def parse_all(self):
        self._parse_binning()
        self._parse_detector()
        self._parse_detector_configuration()
        self._parse_gravity()
        self._parse_instrument_configuration()
        self._parse_mask()
        self._parse_normalisation()
        self._parse_q_resolution()
        self._parse_reduction()
        self._parse_spatial_masks()
        self._parse_transmission()
        self._parse_transmission_fitting()

    @property
    def instrument(self):
        # Use impl so that it throws as mandatory key we later rely on
        try:
            instrument = self._get_val_impl(["instrument", "name"], dict_to_parse=self._input)
        except KeyError:
            raise RuntimeError("instrument.name is missing")
        return SANSInstrument(instrument)

    def _create_state_objs(self):
        self.adjustment = StateAdjustment()
        self.compatibility = StateCompatibility()
        self.convert_to_q = StateConvertToQ()
        self.calculate_transmission = get_calculate_transmission(instrument=self.instrument)
        self.mask = get_mask_builder(data_info=self.data_info).build()
        self.move = get_move_builder(data_info=self.data_info).build()
        self.normalize_to_monitor = get_normalize_to_monitor_builder(data_info=self.data_info).build()
        self.reduction_mode = StateReductionMode()
        self.scale = StateScale()
        self.wavelength = StateWavelength()
        self.wavelength_and_pixel = StateWavelengthAndPixelAdjustment()

        # Ensure they are linked up correctly
        self.adjustment.calculate_transmission = self.calculate_transmission
        self.adjustment.normalize_to_monitor = self.normalize_to_monitor
        self.adjustment.wavelength_and_pixel_adjustment = self.wavelength_and_pixel

    def _parse_instrument_configuration(self):
        inst_config_dict = self._get_val(["instrument", "configuration"])

        self.calculate_transmission.incident_monitor = self._get_val("norm_monitor", inst_config_dict)
        self.calculate_transmission.transmission_monitor = self._get_val("trans_monitor", inst_config_dict)

        self.convert_to_q.q_resolution_collimation_length = self._get_val("collimation_length", inst_config_dict)
        self.convert_to_q.gravity_extra_length = self._get_val("gravity_extra_length", inst_config_dict, 0.0)
        self.convert_to_q.q_resolution_a2 = self._get_val("sample_aperture_diameter", inst_config_dict)

        self.move.sample_offset = self._get_val("sample_offset", inst_config_dict, 0.0)

    def _parse_detector_configuration(self):
        det_config_dict = self._get_val(["detector", "configuration"])

        self.scale.scale = self._get_val("rear_scale", det_config_dict)

        reduction_mode_key = self._get_val(["detector", "configuration", "selected_detector"])
        reduction_mode = ReductionMode(reduction_mode_key) if reduction_mode_key else ReductionMode.NOT_SET
        self.reduction_mode.reduction_mode = reduction_mode

        def update_translations(det_type, values: dict):
            if values:
                self.move.detectors[det_type.value].sample_centre_pos1 = values["x"]
                self.move.detectors[det_type.value].sample_centre_pos2 = values["y"]

        update_translations(DetectorType.HAB, self._get_val("front_centre", det_config_dict))
        update_translations(DetectorType.LAB, self._get_val("rear_centre", det_config_dict))

    def _parse_detector(self):
        detector_dict = self._get_val("detector")
        self.mask.radius_min = self._get_val(["radius_limit", "min"], detector_dict)
        self.mask.radius_max = self._get_val(["radius_limit", "max"], detector_dict)

        calibration_dict = self._get_val("calibration", detector_dict)

        lab_adjustment = self.wavelength_and_pixel.adjustment_files[DetectorType.LAB.value]
        hab_adjustment = self.wavelength_and_pixel.adjustment_files[DetectorType.HAB.value]

        lab_adjustment.wavelength_adjustment_file = self._get_val(["direct", "rear_file"], calibration_dict)
        hab_adjustment.wavelength_adjustment_file = self._get_val(["direct", "front_file"], calibration_dict)

        lab_adjustment.pixel_adjustment_file = self._get_val(["flat", "rear_file"], calibration_dict)
        hab_adjustment.pixel_adjustment_file = self._get_val(["flat", "front_file"], calibration_dict)

        self.adjustment.calibration = self._get_val(["tube", "file"], calibration_dict)

        name_attr_pairs = {"_x": "x_translation_correction",
                           "_y": "y_translation_correction",
                           "_z": "z_translation_correction",
                           "_radius": "radius_correction",
                           "_rot": "rotation_correction",
                           "_side": "side_correction",
                           "_x_tilt": "x_tilt_correction",
                           "_y_tilt": "y_tilt_correction",
                           "_z_tilt": "z_tilt_correction"}

        position_dict = self._get_val("position", calibration_dict)
        lab_move = self.move.detectors[DetectorType.LAB.value]
        hab_move = self.move.detectors[DetectorType.HAB.value]

        for toml_suffix, attr_name in name_attr_pairs.items():
            assert hasattr(lab_move, attr_name)
            assert hasattr(hab_move, attr_name)
            setattr(lab_move, attr_name, self._get_val("rear" + toml_suffix, position_dict, 0.0))
            setattr(hab_move, attr_name, self._get_val("front" + toml_suffix, position_dict, 0.0))

    def _parse_binning(self):
        binning_dict = self._get_val(["binning"])

        def set_wavelength(state_obj):
            wavelength_start = self._get_val(["wavelength", "start"], binning_dict)
            if wavelength_start:
                state_obj.wavelength_low = [wavelength_start]
            wavelength_stop = self._get_val(["wavelength", "stop"], binning_dict)
            if wavelength_stop:
                state_obj.wavelength_high = [wavelength_stop]

            state_obj.wavelength_step = self._get_val(["wavelength", "step"], binning_dict, 0.0)

            step_str = self._get_val(["wavelength", "type"], binning_dict)
            if step_str:
                state_obj.wavelength_step_type = RangeStepType(step_str)

        # TODO we should not have to set the same attributes on all of these things
        set_wavelength(self.calculate_transmission)
        set_wavelength(self.normalize_to_monitor)
        set_wavelength(self.wavelength)
        set_wavelength(self.wavelength_and_pixel)

        one_d_binning = self._get_val(["1d_reduction", "binning"], binning_dict)
        if one_d_binning:
            import pydevd_pycharm
            pydevd_pycharm.settrace('localhost', port=12345, stdoutToServer=True, stderrToServer=True)
            q_min, q_rebin, q_max = self._convert_1d_binning_string(one_d_binning)
            self.convert_to_q.q_min = q_min
            self.convert_to_q.q_1d_rebin_string = q_rebin
            self.convert_to_q.q_max = q_max

        self.convert_to_q.q_xy_max = self._get_val(["2d_reduction", "stop"], binning_dict)
        self.convert_to_q.q_xy_step = self._get_val(["2d_reduction", "step"], binning_dict)
        two_d_step_type = self._get_val(["2d_reduction", "type"], binning_dict)
        if two_d_step_type:
            self.convert_to_q.q_xy_step_type = RangeStepType(two_d_step_type)

    def _parse_reduction(self):
        reduction_dict = self._get_val(["reduction"])

        self.compatibility.time_rebin_string = self._get_val(["events", "binning"], reduction_dict)

        merge_range_dict = self._get_val(["merged", "merge_range"], reduction_dict)
        self.reduction_mode.merge_min = self._get_val("min", merge_range_dict)
        self.reduction_mode.merge_max = self._get_val("max", merge_range_dict)
        self.reduction_mode.merge_mask = self._get_val("use_fit", merge_range_dict)

        # When two max and min values are provided we take the outer bounds
        min_q = []
        max_q = []

        rescale_dict = self._get_val(["merged", "rescale"], reduction_dict)
        min_q.append(self._get_val("min", rescale_dict))
        max_q.append(self._get_val("max", rescale_dict))
        rescale_fit = self._get_val("use_fit", rescale_dict)

        shift_dict = self._get_val(["merged", "shift"], reduction_dict)
        min_q.append(self._get_val("min", shift_dict))
        max_q.append(self._get_val("max", shift_dict))
        shift_fit = self._get_val("use_fit", shift_dict)

        self.reduction_mode.merge_range_min = min(q for q in min_q if q is not None) if any(min_q) else None
        self.reduction_mode.merge_range_max = max(q for q in max_q if q is not None) if any(max_q) else None

        if rescale_fit and shift_fit:
            self.reduction_mode.merge_fit_mode = FitModeForMerge.BOTH
        elif rescale_fit:
            self.reduction_mode.merge_fit_mode = FitModeForMerge.SCALE_ONLY
        elif shift_fit:
            self.reduction_mode.merge_fit_mode = FitModeForMerge.SHIFT_ONLY
        else:
            self.reduction_mode.merge_fit_mode = FitModeForMerge.NO_FIT

    def _parse_q_resolution(self):
        q_dict = self._get_val("q_resolution")
        self.convert_to_q.use_q_resolution = self._get_val("enabled", q_dict)
        self.convert_to_q.moderator_file = self._get_val("moderator_file", q_dict)
        self.convert_to_q.q_resolution_a1 = self._get_val("source_aperture", q_dict)
        self.convert_to_q.q_resolution_delta_r = self._get_val("delta_r", q_dict)

    def _parse_gravity(self):
        self.convert_to_q.use_gravity = self._get_val(["gravity", "enabled"])

    def _parse_transmission(self):
        transmission_dict = self._get_val("transmission")
        monitor_name = self._get_val("selected_monitor", transmission_dict)
        # Have to be a bit more careful since we will use the index operator manually to throw KeyError
        if not transmission_dict or not monitor_name:
            return

        # This is mandatory so we don't use _get_val
        monitor_dict = transmission_dict["monitor"][monitor_name]

        # TODO this is a nasty data structure we should sort out properly by making the
        # monitor number agnostic
        if "M5" in monitor_name:
            self.move.monitor_5_offset = self._get_val("shift", monitor_dict)
        else:
            # Instruments will use monitor 3/4/17788 (not making the last one up) here instead of 4
            self.move.monitor_4_offset = self._get_val("shift", monitor_dict)

        monitor_spec_num = self._get_val("spectrum_number", monitor_dict)
        self.calculate_transmission.transmission_monitor = monitor_spec_num

        if self._get_val("use_own_background", monitor_dict):
            background = monitor_dict["background"]  # Mandatory field when use_own_background
            assert len(background) == 2, "Two background values required"
            self.calculate_transmission.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
            self.calculate_transmission.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})

    def _parse_transmission_fitting(self):
        fit_dict = self._get_val(["transmission", "fitting"])
        if not self._get_val("enabled", fit_dict):
            return

        can_fitting = self.calculate_transmission.fit[DataType.CAN.value]
        sample_fitting = self.calculate_transmission.fit[DataType.SAMPLE.value]

        def set_val_on_both(attr_name, attr_val):
            assert hasattr(can_fitting, attr_name)
            setattr(can_fitting, attr_name, attr_val)
            setattr(sample_fitting, attr_name, attr_val)

        function = str(self._get_val("function", fit_dict))
        fit_type = None
        for enum_val in FitType:
            if str(enum_val.value).casefold() in function.casefold():
                fit_type = enum_val
                break
        if fit_type is None:
            raise KeyError(f"{function} is an unknown fit type")

        set_val_on_both("fit_type", fit_type)
        if fit_type is FitType.POLYNOMIAL:
            parameters = self._get_val("parameters", fit_dict)
            set_val_on_both("polynomial_order", self._get_val("polynomial_order", fit_dict))
            set_val_on_both("wavelength_low", self._get_val("lambda_min", parameters))
            set_val_on_both("wavelength_high", self._get_val("lambda_max", parameters))

    def _parse_normalisation(self):
        normalisation_dict = self._get_val("normalisation")
        selected_monitor = self._get_val("selected_monitor", normalisation_dict)

        if not normalisation_dict or not selected_monitor:
            return

        monitor_dict = normalisation_dict["monitor"][selected_monitor]

        # Mandatory as its subtle if missing
        monitor_spec_num = monitor_dict["spectrum_number"]
        background = monitor_dict["background"]
        assert len(background) == 2, "Two background values required"
        self.calculate_transmission.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
        self.calculate_transmission.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})

    def _parse_spatial_masks(self):
        mask_dict = self._get_val("mask")

        def parse_mask_dict(spatial_dict, bank_type):
            mask_detectors = self.mask.detectors[bank_type.value]
            assert isinstance(mask_detectors, StateMaskDetectors)

            individual_cols = self._get_val("detector_columns", spatial_dict)
            if individual_cols:
                mask_detectors.single_vertical_strip_mask.extend(individual_cols)

            individual_rows = self._get_val("detector_rows", spatial_dict)
            if individual_rows:
                mask_detectors.single_horizontal_strip_mask.extend(individual_rows)

            col_ranges = self._get_val("detector_column_ranges", spatial_dict)
            row_ranges = self._get_val("detector_row_ranges", spatial_dict)
            for pair in col_ranges:
                assert isinstance(pair, list), "Ranges should be entered as lists of lists, e.g. [[1, 10]]"
                assert len(pair) == 2, "A start and end value must exist for each pair"
                mask_detectors.range_vertical_strip_start.append(pair[0])
                mask_detectors.range_vertical_strip_stop.append(pair[1])

            for pair in row_ranges:
                assert isinstance(pair, list), "Ranges should be entered as lists of lists, e.g. [[1, 10]]"
                assert len(pair) == 2, "A start and end value must exist for each pair"
                mask_detectors.range_horizontal_strip_start.append(pair[0])
                mask_detectors.range_horizontal_strip_stop.append(pair[1])

        rear_dict = self._get_val(["spatial", "rear"], mask_dict)
        if rear_dict:
            parse_mask_dict(rear_dict, DetectorType.LAB)

        front_dict = self._get_val(["spatial", "front"], mask_dict)
        if front_dict:
            parse_mask_dict(front_dict, DetectorType.HAB)

    def _parse_mask(self):
        mask_dict = self._get_val("mask")
        self.mask.beam_stop_arm_angle = self._get_val(["beamstop_shadow", "angle"], mask_dict)
        self.mask.beam_stop_arm_width = self._get_val(["beamstop_shadow", "width"], mask_dict)

        mask_files = self._get_val("mask_files", mask_dict)
        if mask_files:
            self.mask.mask_files.extend(mask_files)

        mask_pixels = self._get_val("mask_pixels", mask_dict)
        if mask_pixels:
            for pixel in mask_pixels:
                # TODO we shouldn't be trying to guess which bank each pixel belongs to
                bank = get_bank_for_spectrum_number(pixel, instrument=self.instrument)
                self.mask.detectors[bank.value].single_spectra.append(pixel)

        tof_masks = self._get_val(["time", "tof"], mask_dict)
        if tof_masks:
            for mask_pair in tof_masks:
                self.mask.bin_mask_general_start.append(mask_pair["start"])
                self.mask.bin_mask_general_stop.append(mask_pair["stop"])

    @staticmethod
    def _convert_1d_binning_string(one_d_binning: str):
        # TODO: We have to do some special parsing for this type on behalf of the sans codebase
        # TODO: which should do this instead of the parser
        bin_values = one_d_binning.split(",")

        if len(bin_values) == 3:
            return float(bin_values[0]), bin_values[1], float(bin_values[-1])
        elif len(bin_values) == 5:
            rebin_str = ','.join(bin_values[1:-1])
            return float(bin_values[0]), rebin_str, float(bin_values[-1])
        else:
            raise ValueError("Three or five comma seperated binning values are needed, got {0}".format(one_d_binning))

    def _get_val(self, keys, dict_to_parse=None, default_return=None):
        """
        Gets a nested value within the specified dictionary
        :param keys: A list of keys to iterate through the dictionary
        :param dict_to_parse: (Optional) The dict to parse, if None parses the input dict
        :return: The corresponding value
        """
        if isinstance(keys, str):
            keys = [keys]

        try:
            return self._get_val_impl(keys=keys, dict_to_parse=dict_to_parse)
        except KeyError:
            return default_return

    def _get_val_impl(self, keys, dict_to_parse):
        if dict_to_parse is None:
            dict_to_parse = self._input

        assert isinstance(dict_to_parse, dict)

        val = dict_to_parse[keys[0]]
        if isinstance(val, dict) and len(keys) > 1:
            return self._get_val_impl(keys=keys[1:], dict_to_parse=val)
        return val
