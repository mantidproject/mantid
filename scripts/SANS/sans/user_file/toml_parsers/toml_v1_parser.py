# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Optional

from sans.common.enums import SANSInstrument, ReductionMode, DetectorType, RangeStepType, FitModeForMerge, DataType, FitType, RebinType
from sans.common.general_functions import get_bank_for_spectrum_number, get_detector_types_from_instrument
from sans.state.IStateParser import IStateParser
from sans.state.StateObjects.StateAdjustment import StateAdjustment
from sans.state.StateObjects.StateCalculateTransmission import get_calculate_transmission
from sans.state.StateObjects.StateCompatibility import StateCompatibility
from sans.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans.state.StateObjects.StateData import StateData
from sans.state.StateObjects.StateMaskDetectors import get_mask_builder, StateMaskDetectors
from sans.state.StateObjects.StateMoveDetectors import get_move_builder
from sans.state.StateObjects.StateNormalizeToMonitor import get_normalize_to_monitor_builder
from sans.state.StateObjects.StatePolarization import StatePolarization, StateComponent
from sans.state.StateObjects.StateReductionMode import StateReductionMode
from sans.state.StateObjects.StateSave import StateSave
from sans.state.StateObjects.StateScale import StateScale
from sans.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans.state.StateObjects.StateWavelength import StateWavelength
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import get_wavelength_and_pixel_adjustment_builder
from sans.user_file.parser_helpers.toml_parser_impl_base import TomlParserImplBase
from sans.user_file.parser_helpers.wavelength_parser import DuplicateWavelengthStates, WavelengthTomlParser
from sans.user_file.toml_parsers.toml_v1_schema import TomlSchemaV1Validator


class TomlV1Parser(IStateParser):
    def __init__(self, dict_to_parse, file_information, schema_validator=None):
        self._validator = schema_validator if schema_validator else TomlSchemaV1Validator(dict_to_parse)
        self._validator.validate()

        self._implementation = None
        data_info = self.get_state_data(file_information)
        self._implementation = self._get_impl(dict_to_parse, data_info)
        self._implementation.parse_all()

    @staticmethod
    def _get_impl(*args):
        # Wrapper which can replaced with a mock
        return _TomlV1ParserImpl(*args)

    def get_state_data(self, file_information):
        state_data = super().get_state_data(file_information)
        if self._implementation:
            # Always take the instrument from the TOML file rather than guessing in the new parser
            state_data.instrument = self._implementation.instrument
        return state_data

    def get_state_adjustment(self, _):
        return self._implementation.adjustment

    def get_state_calculate_transmission(self):
        return self._implementation.calculate_transmission

    def get_state_compatibility(self):
        return self._implementation.compatibility

    def get_state_convert_to_q(self):
        return self._implementation.convert_to_q

    def get_state_mask(self, _):
        return self._implementation.mask

    def get_state_move(self, _):
        return self._implementation.move

    def get_state_normalize_to_monitor(self, _):
        return self._implementation.normalize_to_monitor

    def get_state_reduction_mode(self):
        return self._implementation.reduction_mode

    def get_state_polarization(self) -> Optional[StatePolarization]:
        return self._implementation.polarization

    def get_state_save(self):
        return StateSave()

    def get_state_scale(self, file_information):
        scale = self._implementation.scale
        if file_information:
            scale.set_geometry_from_file(file_information)
        return scale

    def get_state_slice_event(self):
        return StateSliceEvent()

    def get_state_wavelength(self):
        return self._implementation.wavelength

    def get_state_wavelength_and_pixel_adjustment(self):
        return self._implementation.wavelength_and_pixel


class _TomlV1ParserImpl(TomlParserImplBase):
    def __init__(self, input_dict, data_info: StateData):
        super(_TomlV1ParserImpl, self).__init__(toml_dict=input_dict)
        # Always take the instrument from the TOML file rather than guessing in the new parser
        data_info.instrument = self.instrument
        self._create_state_objs(data_info=data_info)

    def parse_all(self):
        self._parse_binning()
        self._parse_detector()
        self._parse_detector_configuration()
        self._parse_instrument_configuration()
        self._parse_mask()
        self._parse_normalisation()
        self._parse_q_resolution()
        self._parse_reduction()
        self._parse_spatial_masks()
        self._parse_transmission()
        self._parse_transmission_roi()
        self._parse_transmission_fitting()
        self._parse_polarization()

    @property
    def instrument(self):
        # Use impl so that it throws as mandatory key we later rely on
        try:
            instrument = self._get_val_impl(["instrument", "name"], dict_to_parse=self._input)
        except KeyError:
            raise KeyError("instrument.name is missing")
        return SANSInstrument(instrument)

    def _create_state_objs(self, data_info):
        self.adjustment = StateAdjustment()
        self.compatibility = StateCompatibility()
        self.convert_to_q = StateConvertToQ()
        self.calculate_transmission = get_calculate_transmission(instrument=self.instrument)
        self.mask = get_mask_builder(data_info=data_info).build()
        self.move = get_move_builder(data_info=data_info).build()
        self.normalize_to_monitor = get_normalize_to_monitor_builder(data_info=data_info).build()
        self.reduction_mode = StateReductionMode()
        self.scale = StateScale()
        self.wavelength = StateWavelength()
        self.wavelength_and_pixel = get_wavelength_and_pixel_adjustment_builder(data_info=data_info).build()
        self.polarization = StatePolarization()

        # Ensure they are linked up correctly
        self.adjustment.calculate_transmission = self.calculate_transmission
        self.adjustment.normalize_to_monitor = self.normalize_to_monitor
        self.adjustment.wavelength_and_pixel_adjustment = self.wavelength_and_pixel

    def _parse_instrument_configuration(self):
        inst_config_dict = self.get_val(["instrument", "configuration"])

        self.convert_to_q.q_resolution_collimation_length = self.get_val("collimation_length", inst_config_dict)
        self.convert_to_q.gravity_extra_length = self.get_val("gravity_extra_length", inst_config_dict, 0.0)
        self.convert_to_q.q_resolution_a2 = self.get_val("sample_aperture_diameter", inst_config_dict)

        self.move.sample_offset = self.get_val("sample_offset", inst_config_dict, 0.0)
        self.convert_to_q.use_gravity = self.get_val("gravity_enabled", inst_config_dict, default=True)

    def _parse_detector_configuration(self):
        det_config_dict = self.get_val(["detector", "configuration"])

        self.scale.scale = self.get_val("rear_scale", det_config_dict)

        reduction_mode_key = self.get_mandatory_val(["detector", "configuration", "selected_detector"])
        # LAB/Rear was set by default in user parser, so we fall-back to this
        self.reduction_mode.reduction_mode = ReductionMode.convert(reduction_mode_key, support_deprecated=False)
        self._parse_centre_pos(det_config_dict)

    def _parse_centre_pos(self, det_config_dict):
        def update_translations(det_type, values: dict):
            if values:
                self.move.detectors[det_type.value].sample_centre_pos1 = values["x"]
                self.move.detectors[det_type.value].sample_centre_pos2 = values["y"]

        rear = self.get_val("rear_centre", det_config_dict)
        front = self.get_val("front_centre", det_config_dict)
        all = self.get_val("all_centre", det_config_dict)

        if self.reduction_mode.reduction_mode in [ReductionMode.LAB, ReductionMode.HAB]:
            update_translations(DetectorType.LAB, rear)
            if DetectorType.HAB in get_detector_types_from_instrument(self.instrument):
                update_translations(DetectorType.HAB, front)
        elif self.reduction_mode.reduction_mode in [ReductionMode.ALL, ReductionMode.MERGED]:
            if self.instrument is SANSInstrument.LOQ:
                update_translations(DetectorType.LAB, rear)
                update_translations(DetectorType.HAB, front)
            else:
                update_translations(DetectorType.LAB, all)
                update_translations(DetectorType.HAB, all)

    def _parse_detector(self):
        detector_dict = self.get_val("detector")
        self.mask.radius_min = self.get_val(["radius_limit", "min"], detector_dict)
        self.mask.radius_max = self.get_val(["radius_limit", "max"], detector_dict)

        correction_dict = self.get_val("correction", detector_dict)

        lab_adjustment = self.wavelength_and_pixel.adjustment_files[DetectorType.LAB.value]
        hab_adjustment = self.wavelength_and_pixel.adjustment_files[DetectorType.HAB.value]

        lab_adjustment.wavelength_adjustment_file = self.get_val(["direct", "rear_file"], correction_dict)
        hab_adjustment.wavelength_adjustment_file = self.get_val(["direct", "front_file"], correction_dict)

        lab_adjustment.pixel_adjustment_file = self.get_val(["flat", "rear_file"], correction_dict)
        hab_adjustment.pixel_adjustment_file = self.get_val(["flat", "front_file"], correction_dict)

        self.adjustment.calibration = self.get_val(["tube", "file"], correction_dict)

        name_attr_pairs = {
            "_x": "x_translation_correction",
            "_y": "y_translation_correction",
            "_z": "z_translation_correction",
            "_radius": "radius_correction",
            "_rot": "rotation_correction",
            "_side": "side_correction",
            "_x_tilt": "x_tilt_correction",
            "_y_tilt": "y_tilt_correction",
            "_z_tilt": "z_tilt_correction",
        }

        position_dict = self.get_val("position", correction_dict)
        lab_move = self.move.detectors[DetectorType.LAB.value]
        # Some detectors do not have HAB
        hab_move = self.move.detectors.get(DetectorType.HAB.value, None)

        for toml_suffix, attr_name in name_attr_pairs.items():
            setattr(lab_move, attr_name, self.get_val("rear" + toml_suffix, position_dict, 0.0))
            if hab_move:
                setattr(hab_move, attr_name, self.get_val("front" + toml_suffix, position_dict, 0.0))

    def _parse_binning(self):
        binning_dict = self.get_val(["binning"])

        to_set = DuplicateWavelengthStates(
            transmission=self.calculate_transmission, wavelength=self.wavelength, pixel=self.wavelength_and_pixel
        )

        WavelengthTomlParser(toml_dict=self._input).set_wavelength_details(state_objs=to_set)

        one_d_dict = self.get_val("1d_reduction", binning_dict)
        if one_d_dict:
            one_d_binning = self.get_val(["1d_reduction", "binning"], binning_dict)
            q_min, q_max = self._get_1d_min_max(one_d_binning)
            self.convert_to_q.q_min = q_min
            self.convert_to_q.q_1d_rebin_string = one_d_binning
            self.convert_to_q.q_max = q_max
            self.convert_to_q.radius_cutoff = self.get_val("radius_cut", one_d_dict, default=0.0)
            self.convert_to_q.wavelength_cutoff = self.get_val("wavelength_cut", one_d_dict, default=0.0)

        self.convert_to_q.q_xy_max = self.get_val(["2d_reduction", "stop"], binning_dict)
        self.convert_to_q.q_xy_step = self.get_val(["2d_reduction", "step"], binning_dict)
        two_d_step_type = self.get_val(["2d_reduction", "type"], binning_dict)
        if two_d_step_type and two_d_step_type != RangeStepType.LIN.value:
            raise ValueError(f'{two_d_step_type} binning is not supported for 2D reductions. The type must be set to "Lin".')

        # We could previous interpolate. This is now obsolete, see the docs for details
        self.calculate_transmission.rebin_type = RebinType.REBIN
        self.normalize_to_monitor.rebin_type = RebinType.REBIN

    def _parse_reduction(self):
        reduction_dict = self.get_val(["reduction"])

        events_binning = self.get_val(["events", "binning"], reduction_dict, default="")
        if (events_binning and len(events_binning.split(",")) != 3) or self.instrument is SANSInstrument.ZOOM and not events_binning:
            raise ValueError(f"Events.binning: Three comma separated values are required, got '{events_binning}'")
        self.compatibility.time_rebin_string = events_binning

        merge_range_dict = self.get_val(["merged", "merge_range"], reduction_dict)
        self.reduction_mode.merge_min = self.get_val("min", merge_range_dict)
        self.reduction_mode.merge_max = self.get_val("max", merge_range_dict)
        self.reduction_mode.merge_mask = self.get_val("use_fit", merge_range_dict, default=False)

        # When two max and min values are provided we take the outer bounds
        min_q = []
        max_q = []

        rescale_dict = self.get_val(["merged", "rescale"], reduction_dict)
        rescale_fit = self.get_val("use_fit", rescale_dict)
        if rescale_fit:
            min_q.append(self.get_val("min", rescale_dict))
            max_q.append(self.get_val("max", rescale_dict))
        else:
            self.reduction_mode.merge_scale = self.get_val("factor", rescale_dict, default=1.0)

        shift_dict = self.get_val(["merged", "shift"], reduction_dict)
        shift_fit = self.get_val("use_fit", shift_dict)
        if shift_fit:
            min_q.append(self.get_val("min", shift_dict))
            max_q.append(self.get_val("max", shift_dict))
        else:
            self.reduction_mode.merge_shift = self.get_val("factor", shift_dict, default=0.0)

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
        q_dict = self.get_val("q_resolution")
        self.convert_to_q.use_q_resolution = self.get_val("enabled", q_dict, default=False)
        self.convert_to_q.moderator_file = self.get_val("moderator_file", q_dict)
        self.convert_to_q.q_resolution_a1 = self.get_val("source_aperture", q_dict)
        self.convert_to_q.q_resolution_delta_r = self.get_val("delta_r", q_dict)
        self.convert_to_q.q_resolution_h1 = self.get_val("h1", q_dict)
        self.convert_to_q.q_resolution_h2 = self.get_val("h2", q_dict)
        self.convert_to_q.q_resolution_w1 = self.get_val("w1", q_dict)
        self.convert_to_q.q_resolution_w2 = self.get_val("w2", q_dict)

    def _parse_transmission(self):
        # The legacy user file would accept missing spec nums, we want to lock this down in the
        # TOML parser without affecting backwards compatibility by doing this check upstream
        selected_trans_monitor = self.get_val(["instrument", "configuration", "trans_monitor"])
        if not selected_trans_monitor or str(selected_trans_monitor).casefold() == "ROI".casefold():
            # ROI is handled in _parse_transmission_roi()
            return
        transmission_dict = self.get_mandatory_val(["transmission", "monitor", selected_trans_monitor])

        if self.get_val("use_different_norm_monitor", transmission_dict):
            selected_norm_monitor = self.get_mandatory_val(["transmission", "monitor", selected_trans_monitor, "trans_norm_monitor"])
        else:
            selected_norm_monitor = self.get_mandatory_val(["instrument", "configuration", "norm_monitor"])

        self.calculate_transmission.incident_monitor = self.get_mandatory_val(
            [self._get_normalisation_spelling(), "monitor", selected_norm_monitor, "spectrum_number"]
        )

        self.calculate_transmission.transmission_monitor = self.get_mandatory_val(
            ["transmission", "monitor", selected_trans_monitor, "spectrum_number"]
        )

        monitor_dict = transmission_dict

        if "M5" in selected_trans_monitor:
            self.move.monitor_5_offset = self.get_val("shift", monitor_dict, default=0.0)
        else:
            # Instruments will use monitor 3/4/17788 (not making the last one up) here instead of 4
            self.move.monitor_4_offset = self.get_val("shift", monitor_dict, default=0.0)

        monitor_spec_num = self.get_val("spectrum_number", monitor_dict)
        self.calculate_transmission.transmission_monitor = monitor_spec_num

        if self.get_val("use_own_background", monitor_dict):
            background = monitor_dict["background"]  # Mandatory field when use_own_background
            if len(background) != 2:
                raise ValueError("Two background values required")
            self.calculate_transmission.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
            self.calculate_transmission.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})
            self.normalize_to_monitor.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
            self.normalize_to_monitor.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})

    def _parse_transmission_roi(self):
        monitor_name = self.get_val(["instrument", "configuration", "trans_monitor"])
        if not str(monitor_name).casefold() == "ROI".casefold():
            return

        file = self.get_mandatory_val(["transmission", "ROI", "file"])
        if not isinstance(file, str):
            raise ValueError("A single file is currently only accepted for ROI")
        elif not file:
            raise ValueError("The ROI filename selected was empty")

        self.calculate_transmission.transmission_roi_files = [file]

    def _parse_transmission_fitting(self):
        fit_dict = self.get_val(["transmission", "fitting"])
        can_fitting = self.calculate_transmission.fit[DataType.CAN.value]
        sample_fitting = self.calculate_transmission.fit[DataType.SAMPLE.value]

        def set_val_on_both(attr_name, attr_val):
            assert hasattr(can_fitting, attr_name)
            setattr(can_fitting, attr_name, attr_val)
            setattr(sample_fitting, attr_name, attr_val)

        if not self.get_val("enabled", fit_dict):
            set_val_on_both("fit_type", FitType.NO_FIT)
            return

        function = str(self.get_val("function", fit_dict))
        fit_type = None
        for enum_val in FitType:
            if str(enum_val.value).casefold() in function.casefold():
                fit_type = enum_val
                break
        if fit_type is None:
            raise KeyError(f"{function} is an unknown fit type")

        set_val_on_both("fit_type", fit_type)
        parameters = self.get_val("parameters", fit_dict)
        set_val_on_both("wavelength_low", self.get_val("lambda_min", parameters))
        set_val_on_both("wavelength_high", self.get_val("lambda_max", parameters))
        if fit_type is FitType.POLYNOMIAL:
            set_val_on_both("polynomial_order", self.get_val("polynomial_order", fit_dict))

    def _get_normalisation_spelling(self) -> str:
        return "normalization" if self.get_val("normalization") else "normalisation"

    def _parse_normalisation(self):
        norm_key_name = self._get_normalisation_spelling()
        normalisation_dict = self.get_val(norm_key_name)

        selected_monitor = self.get_val(["instrument", "configuration", "norm_monitor"])

        if self.get_val(["all_monitors", "enabled"], normalisation_dict):
            background = self.get_val(["all_monitors", "background"], normalisation_dict)
            if len(background) != 2:
                raise ValueError("Two background values required")
            self.calculate_transmission.background_TOF_general_start = background[0]
            self.calculate_transmission.background_TOF_general_stop = background[1]
            self.normalize_to_monitor.background_TOF_general_start = background[0]
            self.normalize_to_monitor.background_TOF_general_stop = background[1]

        if not selected_monitor:
            return

        monitor_dict = self.get_mandatory_val([norm_key_name, "monitor", selected_monitor])

        # Mandatory as its subtle if missing
        monitor_spec_num = monitor_dict["spectrum_number"]
        background = self.get_val("background", monitor_dict)

        self.normalize_to_monitor.incident_monitor = monitor_spec_num

        if background:
            if len(background) != 2:
                raise ValueError("Two background values required")
            self.calculate_transmission.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
            self.calculate_transmission.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})
            self.normalize_to_monitor.background_TOF_monitor_start.update({str(monitor_spec_num): background[0]})
            self.normalize_to_monitor.background_TOF_monitor_stop.update({str(monitor_spec_num): background[1]})

    def _parse_spatial_masks(self):
        spatial_dict = self.get_val(["mask", "spatial"])

        def parse_mask_dict(spatial_dict, bank_type):
            mask_detectors = self.mask.detectors[bank_type.value]
            assert isinstance(mask_detectors, StateMaskDetectors)

            individual_cols = self.get_val("detector_columns", spatial_dict)
            if individual_cols:
                mask_detectors.single_vertical_strip_mask.extend(individual_cols)

            individual_rows = self.get_val("detector_rows", spatial_dict)
            if individual_rows:
                mask_detectors.single_horizontal_strip_mask.extend(individual_rows)

            col_ranges = self.get_val("detector_column_ranges", spatial_dict, default=[])
            row_ranges = self.get_val("detector_row_ranges", spatial_dict, default=[])
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

        rear_dict = self.get_val("rear", spatial_dict)
        if rear_dict:
            parse_mask_dict(rear_dict, DetectorType.LAB)

        front_dict = self.get_val("front", spatial_dict)
        if front_dict:
            parse_mask_dict(front_dict, DetectorType.HAB)

        self.mask.beam_stop_arm_angle = self.get_val(["beamstop_shadow", "angle"], spatial_dict)
        self.mask.beam_stop_arm_width = self.get_val(["beamstop_shadow", "width"], spatial_dict)
        self.mask.beam_stop_arm_pos1 = self.get_val(["beamstop_shadow", "x_pos"], spatial_dict, 0.0)
        self.mask.beam_stop_arm_pos2 = self.get_val(["beamstop_shadow", "y_pos"], spatial_dict, 0.0)

        mask_pixels = self.get_val("mask_pixels", spatial_dict)
        if mask_pixels:
            for pixel in mask_pixels:
                # TODO we shouldn't be trying to guess which bank each pixel belongs to
                bank = get_bank_for_spectrum_number(pixel, instrument=self.instrument)
                self.mask.detectors[bank.value].single_spectra.append(pixel)

    def _parse_mask(self):
        mask_dict = self.get_val("mask")

        prompt_peak_vals = self.get_val("prompt_peak", mask_dict)
        self.calculate_transmission.prompt_peak_correction_enabled = bool(prompt_peak_vals)
        self.normalize_to_monitor.prompt_peak_correction_enabled = bool(prompt_peak_vals)
        if prompt_peak_vals:
            pp_min = self.get_val("start", prompt_peak_vals)
            pp_max = self.get_val("stop", prompt_peak_vals)
            self.calculate_transmission.prompt_peak_correction_min = pp_min
            self.calculate_transmission.prompt_peak_correction_max = pp_max
            self.normalize_to_monitor.prompt_peak_correction_min = pp_min
            self.normalize_to_monitor.prompt_peak_correction_max = pp_max

        mask_files = self.get_val("mask_files", mask_dict)
        if mask_files:
            self.mask.mask_files.extend(mask_files)

        tof_masks = self.get_val(["time", "tof"], mask_dict)
        if tof_masks:
            for mask_pair in tof_masks:
                self.mask.bin_mask_general_start.append(mask_pair["start"])
                self.mask.bin_mask_general_stop.append(mask_pair["stop"])

        phi_mask = self.get_val(["phi"], mask_dict)
        if phi_mask:
            if "mirror" in phi_mask:
                self.mask.use_mask_phi_mirror = phi_mask["mirror"]
            if "start" in phi_mask:
                self.mask.phi_min = phi_mask["start"]
            if "stop" in phi_mask:
                self.mask.phi_max = phi_mask["stop"]

    def _parse_polarization(self):
        polarization_dict = self.get_val("polarization")
        if polarization_dict is None:
            return
        self.polarization.flipper_configuration = self.get_val("flipper_configuration", polarization_dict)
        self.polarization.spin_configuration = self.get_val("spin_configuration", polarization_dict)
        flipper_dicts = self.get_val("flipper", polarization_dict)
        if flipper_dicts:
            for flipper_dict in flipper_dicts.values():
                self.polarization.flippers.append(self._parse_component(flipper_dict))

    def _parse_component(self, component_dict: dict) -> StateComponent:
        component_state = StateComponent()
        if component_dict is None:
            return component_state
        component_state.idf_component_name = self.get_val("idf_component_name", component_dict)
        component_state.device_name = self.get_val("device_name", component_dict)
        component_state.device_type = self.get_val("device_type", component_dict)
        location_dict = self.get_val("location", component_dict)
        if location_dict:
            component_state.location_x = self.get_val("x", location_dict)
            component_state.location_y = self.get_val("y", location_dict)
            component_state.location_z = self.get_val("z", location_dict)
        component_state.transmission = self.get_val("transmission", component_dict)
        component_state.efficiency = self.get_val("efficiency", component_dict)
        return component_state

    @staticmethod
    def _get_1d_min_max(one_d_binning: str):
        # TODO: We have to do some special parsing for this type on behalf of the sans codebase
        # TODO: which should do this instead of the parser
        bin_values = one_d_binning.split(",")

        if len(bin_values) == 3:
            return float(bin_values[0]), float(bin_values[-1])
        elif len(bin_values) == 5:
            return float(bin_values[0]), float(bin_values[-1])
        else:
            raise ValueError("Three or five comma separated binning values are needed, got {0}".format(one_d_binning))
