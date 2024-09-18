# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from typing import List, Dict
from unittest import mock

from sans.common.enums import (
    SANSInstrument,
    SANSFacility,
    DetectorType,
    ReductionMode,
    RangeStepType,
    FitModeForMerge,
    DataType,
    FitType,
    RebinType,
)
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateMaskDetectors import StateMaskDetectors, StateMask
from sans.test_helper.file_information_mock import SANSFileInformationMock
from sans.user_file.parser_helpers.toml_parser_impl_base import MissingMandatoryParam
from sans.user_file.toml_parsers.toml_v1_parser import TomlV1Parser


class TomlV1ParserTest(unittest.TestCase):
    @staticmethod
    def _get_mock_data_info():
        # TODO I really really dislike having to do this in a test, but
        # TODO de-coupling StateData is required to avoid it
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024)
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_builder.set_sample_scatter_period(3)
        return data_builder.build()

    def _setup_parser(self, dict_vals) -> TomlV1Parser:
        def _add_missing_mandatory_key(dict_to_check: Dict, key_path: List[str], replacement_val):
            _dict = dict_to_check
            for key in key_path[0:-1]:
                if key not in _dict:
                    _dict[key] = {}
                _dict = _dict[key]

            if key_path[-1] not in _dict:
                _dict[key_path[-1]] = replacement_val  # Add in child value
            return dict_to_check

        self._mocked_data_info = self._get_mock_data_info()
        # instrument key needs to generally be present
        dict_vals = _add_missing_mandatory_key(dict_vals, ["instrument", "name"], "LOQ")
        dict_vals = _add_missing_mandatory_key(dict_vals, ["detector", "configuration", "selected_detector"], "rear")

        return TomlV1Parser(dict_vals, file_information=None)

    def test_instrument(self):
        parser = self._setup_parser(dict_vals={"instrument": {"name": SANSInstrument.SANS2D.value}})
        inst = parser._implementation.instrument
        self.assertTrue(inst is SANSInstrument.SANS2D, msg="Got %r instead" % inst)

    def test_validate_is_called_on_init(self):
        schema_validator = mock.Mock()

        # No implementation needed
        with mock.patch("sans.user_file.toml_parsers.toml_v1_parser.TomlV1Parser._get_impl"):
            TomlV1Parser(dict_to_parse=None, schema_validator=schema_validator, file_information=None)
            self.assertTrue(schema_validator.validate.called)

    def _loop_over_supported_keys(self, supported_keys, top_level_keys):
        top_level_dict = {}
        dict_view = top_level_dict

        # We need to append a mock object using the bottom key in the below loop
        for key in top_level_keys[:-1]:
            dict_view[key] = {}
            dict_view = dict_view[key]

        for k, func in supported_keys:
            expected = mock.NonCallableMock()
            dict_view[top_level_keys[-1]] = {k: expected}
            parser = self._setup_parser(top_level_dict)

            val = func(parser)
            self.assertEqual(expected, val, "Failed to get key {0}".format(k))

    def test_instrument_configuration_parsed(self):
        supported_keys = [
            ("collimation_length", lambda x: x.get_state_convert_to_q().q_resolution_collimation_length),
            ("gravity_extra_length", lambda x: x.get_state_convert_to_q().gravity_extra_length),
            ("sample_aperture_diameter", lambda x: x.get_state_convert_to_q().q_resolution_a2),
            ("sample_offset", lambda x: x.get_state_move(None).sample_offset),
            ("gravity_enabled", lambda x: x.get_state_convert_to_q().use_gravity),
        ]

        self._loop_over_supported_keys(supported_keys=supported_keys, top_level_keys=["instrument", "configuration"])

    def test_detector_configuration_parsed(self):
        supported_keys = [
            ("rear_scale", lambda x: x.get_state_scale(file_information=None).scale),
            # ("front_scale", lambda x: x.get_state_scale()) TODO this is issue # 27948
        ]
        self._loop_over_supported_keys(supported_keys=supported_keys, top_level_keys=["detector", "configuration"])

        top_level_dict = {"detector": {"configuration": {}}}
        config_dict = top_level_dict["detector"]["configuration"]

        expected_reduction_mode = ReductionMode.ALL
        config_dict["selected_detector"] = expected_reduction_mode.value
        config_dict["front_centre"] = {"x": 1, "y": 2}
        config_dict["rear_centre"] = {"x": 2, "y": 3}

        def get_beam_position(state_move, bank_enum):
            x = state_move.detectors[bank_enum.value].sample_centre_pos1
            y = state_move.detectors[bank_enum.value].sample_centre_pos2
            return x, y

        parser = self._setup_parser(dict_vals=top_level_dict)
        state_move = parser.get_state_move(None)

        self.assertTrue(parser.get_state_reduction_mode().reduction_mode is expected_reduction_mode)
        self.assertEqual((1, 2), get_beam_position(state_move, DetectorType.HAB))
        self.assertEqual((2, 3), get_beam_position(state_move, DetectorType.LAB))

    def test_all_centre_entry(self):
        input_dict = {
            "detector": {"configuration": {"all_centre": {"x": 2, "y": 3.4}, "selected_detector": "all"}},
            "instrument": {"name": "SANS2D"},
        }
        parser = self._setup_parser(input_dict)
        for i in [ReductionMode.HAB, ReductionMode.LAB]:
            move = parser.get_state_move(None)
            self.assertEqual(2, move.detectors[i.value].sample_centre_pos1)
            self.assertEqual(3.4, move.detectors[i.value].sample_centre_pos2)

    def test_loq_uses_front_and_rear_not_all_centre(self):
        input_dict = {
            "detector": {
                "configuration": {
                    "all_centre": {"x": 2, "y": 3.4},
                    "front_centre": {"x": 2.1, "y": 3.5},
                    "rear_centre": {"x": 2.2, "y": 3.6},
                    "selected_detector": "all",
                }
            },
            "instrument": {"name": "LOQ"},
        }
        parser = self._setup_parser(input_dict)
        move = parser.get_state_move(None)
        self.assertEqual(2.1, move.detectors[ReductionMode.HAB.value].sample_centre_pos1)
        self.assertEqual(3.5, move.detectors[ReductionMode.HAB.value].sample_centre_pos2)
        self.assertEqual(2.2, move.detectors[ReductionMode.LAB.value].sample_centre_pos1)
        self.assertEqual(3.6, move.detectors[ReductionMode.LAB.value].sample_centre_pos2)

    def test_sets_rear_and_front_centre_when_rear_detector_selected(self):
        input_dict = {
            "detector": {
                "configuration": {
                    "all_centre": {"x": 2, "y": 3.4},
                    "front_centre": {"x": 2.1, "y": 3.5},
                    "rear_centre": {"x": 2.2, "y": 3.6},
                    "selected_detector": "rear",
                }
            }
        }
        parser = self._setup_parser(input_dict)
        move = parser.get_state_move(None)
        self.assertEqual(2.1, move.detectors[ReductionMode.HAB.value].sample_centre_pos1)
        self.assertEqual(3.5, move.detectors[ReductionMode.HAB.value].sample_centre_pos2)
        self.assertEqual(2.2, move.detectors[ReductionMode.LAB.value].sample_centre_pos1)
        self.assertEqual(3.6, move.detectors[ReductionMode.LAB.value].sample_centre_pos2)

    def test_sets_front_and_rear_centre_when_front_detector_selected(self):
        input_dict = {
            "detector": {
                "configuration": {
                    "all_centre": {"x": 2, "y": 3.4},
                    "front_centre": {"x": 2.1, "y": 3.5},
                    "rear_centre": {"x": 2.2, "y": 3.6},
                    "selected_detector": "front",
                }
            }
        }
        parser = self._setup_parser(input_dict)
        move = parser.get_state_move(None)
        self.assertEqual(2.1, move.detectors[ReductionMode.HAB.value].sample_centre_pos1)
        self.assertEqual(3.5, move.detectors[ReductionMode.HAB.value].sample_centre_pos2)
        self.assertEqual(2.2, move.detectors[ReductionMode.LAB.value].sample_centre_pos1)
        self.assertEqual(3.6, move.detectors[ReductionMode.LAB.value].sample_centre_pos2)

    def test_loaded_correctly_when_on_single_bank_instrument(self):
        input_dict = {
            "instrument": {"name": "LARMOR"},
            "detector": {
                "configuration": {
                    "all_centre": {"x": 2, "y": 3.4},
                    "front_centre": {"x": 2.1, "y": 3.5},
                    "rear_centre": {"x": 2.2, "y": 3.6},
                    "selected_detector": "front",
                }
            },
        }
        parser = self._setup_parser(input_dict)
        move = parser.get_state_move(None)
        self.assertTrue(ReductionMode.HAB.value not in move.detectors.keys())
        self.assertEqual(2.2, move.detectors[ReductionMode.LAB.value].sample_centre_pos1)
        self.assertEqual(3.6, move.detectors[ReductionMode.LAB.value].sample_centre_pos2)

    def test_rear_front_maps_to_enum_correctly(self):
        for user_input, enum_val in [
            ("rear", ReductionMode.LAB),
            ("front", ReductionMode.HAB),
            ("all", ReductionMode.ALL),
            ("merged", ReductionMode.MERGED),
        ]:
            input_dict = {"detector": {"configuration": {"selected_detector": user_input}}}
            parser = self._setup_parser(dict_vals=input_dict)
            self.assertEqual(enum_val, parser.get_state_reduction_mode().reduction_mode)

    def test_legacy_reduction_mode_rejected(self):
        for legacy_input in ["hab", "lab"]:
            with self.assertRaisesRegex(ValueError, "rear"):
                input_dict = {"detector": {"configuration": {"selected_detector": legacy_input}}}
                self._setup_parser(dict_vals=input_dict)

    def test_reduction_mode_mandatory(self):
        with self.assertRaisesRegex(MissingMandatoryParam, "selected_detector"):
            TomlV1Parser({"instrument": {"name": "LOQ"}}, None)

    def test_binning_commands_parsed(self):
        # Wavelength
        for bin_type in ["Lin", "Log"]:
            wavelength_dict = {"binning": {"wavelength": {"start": 1.1, "step": 0.1, "stop": 2.2, "type": bin_type}}}
            wavelength = self._setup_parser(wavelength_dict).get_state_wavelength()
            self.assertEqual((1.1, 2.2), wavelength.wavelength_interval.wavelength_full_range)
            self.assertEqual(0.1, wavelength.wavelength_interval.wavelength_step)
            self.assertEqual(RangeStepType(bin_type), wavelength.wavelength_step_type)

        one_d_reduction_q_dict = {
            "binning": {"1d_reduction": {"binning": "1.0, 0.1, 2.0, -0.2, 3.0", "radius_cut": 12.3, "wavelength_cut": 23.4}}
        }
        one_d_convert_to_q = self._setup_parser(one_d_reduction_q_dict).get_state_convert_to_q()
        self.assertEqual(1.0, one_d_convert_to_q.q_min)
        self.assertEqual(3.0, one_d_convert_to_q.q_max)
        self.assertEqual("1.0, 0.1, 2.0, -0.2, 3.0", one_d_convert_to_q.q_1d_rebin_string.strip())
        self.assertEqual(12.3, one_d_convert_to_q.radius_cutoff)
        self.assertEqual(23.4, one_d_convert_to_q.wavelength_cutoff)

        two_d_reduction_q_dict = {"binning": {"2d_reduction": {"step": 1.0, "stop": 5.0, "type": "Lin"}}}
        results = self._setup_parser(two_d_reduction_q_dict)
        two_d_convert_to_q = results.get_state_convert_to_q()
        self.assertEqual(5.0, two_d_convert_to_q.q_xy_max)
        self.assertEqual(1.0, two_d_convert_to_q.q_xy_step)
        self.assertTrue(results.get_state_calculate_transmission().rebin_type is RebinType.REBIN)

    def test_binning_commands_ignores_log(self):
        # Wavelength
        for bin_type in ["Lin", "Log"]:
            wavelength_dict = {"binning": {"wavelength": {"start": 1.1, "step": 0.1, "stop": 2.2, "type": bin_type}}}
            wavelength = self._setup_parser(wavelength_dict).get_state_wavelength()
            self.assertEqual((1.1, 2.2), wavelength.wavelength_interval.wavelength_full_range)
            self.assertEqual(0.1, wavelength.wavelength_interval.wavelength_step)
            self.assertEqual(RangeStepType(bin_type), wavelength.wavelength_step_type)

        one_d_reduction_q_dict = {
            "binning": {"1d_reduction": {"binning": "1.0, 0.1, 2.0, -0.2, 3.0", "radius_cut": 12.3, "wavelength_cut": 23.4}}
        }
        one_d_convert_to_q = self._setup_parser(one_d_reduction_q_dict).get_state_convert_to_q()
        self.assertEqual(1.0, one_d_convert_to_q.q_min)
        self.assertEqual(3.0, one_d_convert_to_q.q_max)
        self.assertEqual("1.0, 0.1, 2.0, -0.2, 3.0", one_d_convert_to_q.q_1d_rebin_string.strip())
        self.assertEqual(12.3, one_d_convert_to_q.radius_cutoff)
        self.assertEqual(23.4, one_d_convert_to_q.wavelength_cutoff)

        two_d_reduction_q_dict = {"binning": {"2d_reduction": {"step": 1.0, "stop": 5.0, "type": "Log"}}}
        self.assertRaises(ValueError, self._setup_parser, two_d_reduction_q_dict)

    def test_reduction_commands_parsed(self):
        top_level_dict = {"reduction": {"merged": {}, "events": {}}}

        merged_dict = top_level_dict["reduction"]["merged"]
        merged_dict["merge_range"] = {"min": 1, "max": 2, "use_fit": False}
        merged_dict["rescale"] = {"min": 0.1, "max": 0.2, "use_fit": True}
        merged_dict["shift"] = {"min": 0.3, "max": 0.4, "use_fit": True}

        events_dict = top_level_dict["reduction"]["events"]

        expected_binning = "1,1,10"
        events_dict["binning"] = expected_binning

        parsed_obj = self._setup_parser(top_level_dict)
        self.assertEqual(parsed_obj.get_state_compatibility().time_rebin_string, expected_binning)

        state_reduction = parsed_obj.get_state_reduction_mode()
        self.assertEqual(state_reduction.merge_min, 1)
        self.assertEqual(state_reduction.merge_max, 2)
        self.assertEqual(state_reduction.merge_mask, False)

        # Note this should take the max and min of rescale and shift when both are present
        self.assertEqual(state_reduction.merge_range_min, 0.1)
        self.assertEqual(state_reduction.merge_range_max, 0.4)

        self.assertEqual(state_reduction.merge_fit_mode, FitModeForMerge.BOTH)

    def test_detector_parsed(self):
        top_level_dict = {
            "detector": {"correction": {"direct": {}, "flat": {}, "tube": {}, "position": {}}, "radius_limit": {"min": None, "max": None}}
        }

        correction_dict = top_level_dict["detector"]["correction"]

        direct_front = mock.NonCallableMock()
        direct_rear = mock.NonCallableMock()
        correction_dict["direct"]["front_file"] = direct_front
        correction_dict["direct"]["rear_file"] = direct_rear

        flat_front = mock.NonCallableMock()
        flat_rear = mock.NonCallableMock()
        correction_dict["flat"]["front_file"] = flat_front
        correction_dict["flat"]["rear_file"] = flat_rear

        tube_file = mock.NonCallableMock()
        correction_dict["tube"]["file"] = tube_file

        radius_limit = top_level_dict["detector"]["radius_limit"]
        radius_limit["min"] = 100
        radius_limit["max"] = 200

        parser = self._setup_parser(top_level_dict)
        wavelength_state = parser.get_state_wavelength_and_pixel_adjustment()

        # Where Front = HAB and Rear = LAB in SANS terminology
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.LAB.value].wavelength_adjustment_file, direct_rear)
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.HAB.value].wavelength_adjustment_file, direct_front)

        self.assertEqual(wavelength_state.adjustment_files[DetectorType.LAB.value].pixel_adjustment_file, flat_rear)
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.HAB.value].pixel_adjustment_file, flat_front)

        self.assertEqual(parser.get_state_adjustment(None).calibration, tube_file)

        mask = parser.get_state_mask(None)
        self.assertIsInstance(mask, StateMask)
        self.assertEqual(100, mask.radius_min)
        self.assertEqual(200, mask.radius_max)

    def test_detector_correction_position(self):
        top_level_dict = {"detector": {"correction": {"position": {}}}}
        position_dict = top_level_dict["detector"]["correction"]["position"]

        for adjustment in ["_x", "_y", "_z", "_rot", "_radius", "_side", "_x_tilt", "_y_tilt", "_z_tilt"]:
            position_dict["front" + adjustment] = mock.NonCallableMock()
            position_dict["rear" + adjustment] = mock.NonCallableMock()

        def assert_lab_hab_val(move_state, adjustment_name, state_name):
            lab_val = getattr(move_state.detectors[DetectorType.LAB.value], state_name)
            hab_val = getattr(move_state.detectors[DetectorType.HAB.value], state_name)
            self.assertEqual(position_dict["front" + adjustment_name], hab_val)
            self.assertEqual(position_dict["rear" + adjustment_name], lab_val)

        state_move = self._setup_parser(top_level_dict).get_state_move(None)
        assert_lab_hab_val(state_move, "_x", "x_translation_correction")
        assert_lab_hab_val(state_move, "_y", "y_translation_correction")

    def test_q_resolution(self):
        top_level_dict = {"q_resolution": {}}
        q_resolution_dict = top_level_dict["q_resolution"]

        q_resolution_dict["enabled"] = True
        q_resolution_dict["moderator_file"] = mock.NonCallableMock()
        q_resolution_dict["source_aperture"] = 1
        q_resolution_dict["delta_r"] = 2
        q_resolution_dict["h1"], q_resolution_dict["h2"] = 3, 4
        q_resolution_dict["w1"], q_resolution_dict["w2"] = 5, 6

        q_resolution = self._setup_parser(top_level_dict).get_state_convert_to_q()

        self.assertEqual(True, q_resolution.use_q_resolution)
        self.assertEqual(1, q_resolution.q_resolution_a1)
        self.assertEqual(2, q_resolution.q_resolution_delta_r)
        self.assertEqual(3, q_resolution.q_resolution_h1)
        self.assertEqual(4, q_resolution.q_resolution_h2)
        self.assertEqual(5, q_resolution.q_resolution_w1)
        self.assertEqual(6, q_resolution.q_resolution_w2)
        self.assertEqual(q_resolution_dict["moderator_file"], q_resolution.moderator_file)

    def test_transmission(self):
        top_level_dict = {
            "instrument": {"configuration": {"norm_monitor": "", "trans_monitor": ""}},
            "normalisation": {"monitor": {"M3": {}, "M5": {}}},
            "transmission": {"monitor": {"M3": {}, "M5": {}}},
        }
        monitor_dict = top_level_dict["transmission"]["monitor"]

        m3_dict = monitor_dict["M3"]
        m3_dict["background"] = [100, 200]
        m3_dict["spectrum_number"] = 3
        m3_dict["shift"] = 10
        m3_dict["use_own_background"] = True

        top_level_dict["normalisation"]["monitor"]["M3"]["spectrum_number"] = 100

        m5_dict = monitor_dict["M5"]
        m5_dict["spectrum_number"] = 5
        m5_dict["use_own_background"] = False
        m5_dict["shift"] = -10
        top_level_dict["normalisation"]["monitor"]["M5"]["spectrum_number"] = 200

        top_level_dict["instrument"]["configuration"]["norm_monitor"] = "M3"
        top_level_dict["instrument"]["configuration"]["trans_monitor"] = "M3"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(100, calc_transmission.incident_monitor)
        self.assertEqual(3, calc_transmission.transmission_monitor)
        self.assertEqual(10, parser.get_state_move(None).monitor_4_offset)
        self.assertEqual({"3": 100}, calc_transmission.background_TOF_monitor_start)
        self.assertEqual({"3": 200}, calc_transmission.background_TOF_monitor_stop)

        # Check switching the selected monitor picks up correctly
        top_level_dict["instrument"]["configuration"]["norm_monitor"] = "M5"
        top_level_dict["instrument"]["configuration"]["trans_monitor"] = "M5"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(200, calc_transmission.incident_monitor)
        self.assertEqual(5, calc_transmission.transmission_monitor)
        self.assertEqual(-10, parser.get_state_move(None).monitor_5_offset)
        self.assertFalse(calc_transmission.background_TOF_monitor_start)
        self.assertFalse(calc_transmission.background_TOF_monitor_stop)

        with self.assertRaises(KeyError):
            top_level_dict["instrument"]["configuration"]["trans_monitor"] = "M999"
            self._setup_parser(top_level_dict)

    def test_transmission_with_different_norm_monitor(self):
        # A transmission run can be normalised by a different norm monitor. This is useful for cryostats
        # where a different transmission monitor is used due to physical space limitations on the instrument
        top_level_dict = {
            "instrument": {"configuration": {"norm_monitor": "M3", "trans_monitor": "M3"}},
            "normalisation": {"monitor": {"M3": {}, "M5": {}}},
            "transmission": {"monitor": {"M3": {}}},
        }

        monitor_dict = top_level_dict["transmission"]["monitor"]
        top_level_dict["normalisation"]["monitor"]["M3"]["spectrum_number"] = 3
        top_level_dict["normalisation"]["monitor"]["M5"]["spectrum_number"] = 5

        m3_dict = monitor_dict["M3"]
        m3_dict["spectrum_number"] = 3
        m3_dict["use_different_norm_monitor"] = True

        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

        m3_dict["trans_norm_monitor"] = "M5"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(5, calc_transmission.incident_monitor)
        self.assertEqual(3, calc_transmission.transmission_monitor)

    def test_transmission_monitor_parser_ignores_roi(self):
        top_level_dict = {
            "instrument": {"configuration": {"trans_monitor": "ROI"}},
            "transmission": {"monitor": {"M3": {}}, "ROI": {"file": "foo"}},
        }
        monitor_dict = top_level_dict["transmission"]["monitor"]

        m3_dict = monitor_dict["M3"]
        m3_dict["spectrum_number"] = 3
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(3, calc_transmission.transmission_monitor)

    def test_transmission_monitor_parses_roi(self):
        expected_file_name = "test.xml"
        top_level_dict = {"instrument": {"configuration": {"trans_monitor": "ROI"}}, "transmission": {"ROI": {"file": expected_file_name}}}

        result = self._setup_parser(top_level_dict)
        self.assertEqual([expected_file_name], result.get_state_calculate_transmission().transmission_roi_files)

    def test_transmission_monitor_errors_for_multiple_values(self):
        top_level_dict = {
            "instrument": {"configuration": {"trans_monitor": "ROI"}},
            "transmission": {"ROI": {"file": ["test1.xml", "test2.xml"]}},
        }

        with self.assertRaisesRegex(ValueError, "single file"):
            self._setup_parser(top_level_dict)

    def test_transmission_monitor_errors_for_empty_value(self):
        top_level_dict = {"instrument": {"configuration": {"trans_monitor": "ROI"}}, "transmission": {"ROI": {"file": ""}}}

        with self.assertRaisesRegex(ValueError, "empty"):
            self._setup_parser(top_level_dict)

    def test_transmission_fitting(self):
        top_level_dict = {"transmission": {"fitting": {}}}
        fitting_dict = top_level_dict["transmission"]["fitting"]
        fitting_dict["enabled"] = True
        fitting_dict["function"] = "Polynomial"
        fitting_dict["polynomial_order"] = 5
        fitting_dict["parameters"] = {"lambda_min": 20, "lambda_max": 30}

        def check_can_and_sample(parser, attr_name, expected):
            can = parser.get_state_calculate_transmission().fit[DataType.CAN.value]
            sample = parser.get_state_calculate_transmission().fit[DataType.SAMPLE.value]
            self.assertEqual(expected, getattr(can, attr_name))
            self.assertEqual(expected, getattr(sample, attr_name))

        parser = self._setup_parser(top_level_dict)
        check_can_and_sample(parser, "fit_type", FitType.POLYNOMIAL)
        check_can_and_sample(parser, "polynomial_order", 5)
        check_can_and_sample(parser, "wavelength_low", 20)
        check_can_and_sample(parser, "wavelength_high", 30)

        # Linear / Log should not set the above
        fitting_dict["function"] = "Linear"
        parser = self._setup_parser(top_level_dict)
        check_can_and_sample(parser, "fit_type", FitType.LINEAR)
        check_can_and_sample(parser, "polynomial_order", 0)
        check_can_and_sample(parser, "wavelength_low", 20)
        check_can_and_sample(parser, "wavelength_high", 30)

        fitting_dict["function"] = "NotSet"
        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

        fitting_dict["enabled"] = False
        self.assertIsNotNone(self._setup_parser(top_level_dict))

    def test_normalisation_normalization_both_accepted(self):
        for norm_key in "normalisation", "normalization":
            norm_dict = {"instrument": {"configuration": {"norm_monitor": "M1"}}, norm_key: {"monitor": {"M1": {}, "A2": {}}}}

            monitor_dict = norm_dict[norm_key]["monitor"]

            m1_dict = monitor_dict["M1"]
            m1_dict["background"] = [100, 200]
            m1_dict["spectrum_number"] = 1

            calc_transmission = self._setup_parser(norm_dict).get_state_calculate_transmission()
            self.assertEqual({"1": 100}, calc_transmission.background_TOF_monitor_start)
            self.assertEqual({"1": 200}, calc_transmission.background_TOF_monitor_stop)

    def test_parsing_all_monitor_background(self):
        top_level_dict = {"normalisation": {"all_monitors": {"enabled": True, "background": [1200, 2400]}}}
        parsed = self._setup_parser(top_level_dict)
        parsed_transmission = parsed.get_state_calculate_transmission()
        self.assertEqual(1200, parsed_transmission.background_TOF_general_start)
        self.assertEqual(2400, parsed_transmission.background_TOF_general_stop)
        parsed_norm_monitors = parsed.get_state_normalize_to_monitor(None)
        self.assertEqual(1200, parsed_norm_monitors.background_TOF_general_start)
        self.assertEqual(2400, parsed_norm_monitors.background_TOF_general_stop)

    def test_parsing_all_monitor_background_ignored_false(self):
        top_level_dict = {"normalisation": {"all_monitors": {"enabled": False, "background": [1200, 2400]}}}
        parsed = self._setup_parser(top_level_dict)
        parsed_transmission = parsed.get_state_calculate_transmission()
        self.assertIsNone(parsed_transmission.background_TOF_general_start)
        self.assertIsNone(parsed_transmission.background_TOF_general_stop)
        parsed_norm_monitors = parsed.get_state_normalize_to_monitor(None)
        self.assertIsNone(parsed_norm_monitors.background_TOF_general_start)
        self.assertIsNone(parsed_norm_monitors.background_TOF_general_stop)

    def test_parse_normalisation(self):
        # A2 is intentional to check were not hardcoded to Monitor x (Mx), such that a user could have FooY
        top_level_dict = {"instrument": {"configuration": {"norm_monitor": ""}}, "normalisation": {"monitor": {"M1": {}, "A2": {}}}}

        monitor_dict = top_level_dict["normalisation"]["monitor"]

        m1_dict = monitor_dict["M1"]
        m1_dict["background"] = [100, 200]
        m1_dict["spectrum_number"] = 1

        a2_dict = monitor_dict["A2"]
        a2_dict["background"] = [400, 800]
        a2_dict["spectrum_number"] = 2

        top_level_dict["instrument"]["configuration"]["norm_monitor"] = "M1"
        parser = self._setup_parser(top_level_dict)
        norm_to_monitor = parser.get_state_normalize_to_monitor(None)
        self.assertEqual({"1": 100}, norm_to_monitor.background_TOF_monitor_start)
        self.assertEqual({"1": 200}, norm_to_monitor.background_TOF_monitor_stop)
        self.assertEqual(1, norm_to_monitor.incident_monitor)

        top_level_dict["instrument"]["configuration"]["norm_monitor"] = "A2"
        parser = self._setup_parser(top_level_dict)
        norm_to_monitor = parser.get_state_calculate_transmission()
        self.assertEqual({"2": 400}, norm_to_monitor.background_TOF_monitor_start)
        self.assertEqual({"2": 800}, norm_to_monitor.background_TOF_monitor_stop)
        self.assertEqual(2, norm_to_monitor.incident_monitor)

        top_level_dict["instrument"]["configuration"]["norm_monitor"] = "NotThere"
        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

    def test_parse_mask_spatial(self):
        top_level_dict = {
            "mask": {
                "spatial": {
                    "beamstop_shadow": {},
                    "front": {},
                    "rear": {},
                    "mask_pixels": [],
                }
            }
        }

        rear_spatial_dict = top_level_dict["mask"]["spatial"]["rear"]
        rear_spatial_dict["detector_columns"] = [101, 102]
        rear_spatial_dict["detector_rows"] = [201, 202]
        rear_spatial_dict["detector_column_ranges"] = [[0, 10], [20, 30]]
        rear_spatial_dict["detector_row_ranges"] = [[40, 50]]

        front_spatial_dict = top_level_dict["mask"]["spatial"]["front"]
        front_spatial_dict["detector_columns"] = [1, 2]
        front_spatial_dict["detector_rows"] = [2, 3]
        front_spatial_dict["detector_column_ranges"] = [[0, 10]]
        front_spatial_dict["detector_row_ranges"] = [[100, 400]]

        top_level_dict["mask"]["spatial"]["beamstop_shadow"] = {"width": 10, "angle": 180}
        mask_pixels_expected = [1, 2, 4, 17000]  # 17000 is in HAB on LOQ
        top_level_dict["mask"]["spatial"]["mask_pixels"] = mask_pixels_expected

        mask_state = self._setup_parser(top_level_dict).get_state_mask(None)

        rear_result = mask_state.detectors[DetectorType.LAB.value]
        self.assertIsInstance(rear_result, StateMaskDetectors)
        self.assertEqual([101, 102], rear_result.single_vertical_strip_mask)
        self.assertEqual([201, 202], rear_result.single_horizontal_strip_mask)
        self.assertEqual([0, 20], rear_result.range_vertical_strip_start)
        self.assertEqual([10, 30], rear_result.range_vertical_strip_stop)
        self.assertEqual([40], rear_result.range_horizontal_strip_start)
        self.assertEqual([50], rear_result.range_horizontal_strip_stop)

        front_result = mask_state.detectors[DetectorType.HAB.value]
        self.assertIsInstance(front_result, StateMaskDetectors)
        self.assertEqual([1, 2], front_result.single_vertical_strip_mask)
        self.assertEqual([2, 3], front_result.single_horizontal_strip_mask)
        self.assertEqual([0], front_result.range_vertical_strip_start)
        self.assertEqual([10], front_result.range_vertical_strip_stop)
        self.assertEqual([100], front_result.range_horizontal_strip_start)
        self.assertEqual([400], front_result.range_horizontal_strip_stop)

        # Pixel masks
        self.assertEqual([1, 2, 4], mask_state.detectors[DetectorType.LAB.value].single_spectra)
        self.assertEqual([17000], mask_state.detectors[DetectorType.HAB.value].single_spectra)
        # Beamstop angle
        self.assertEqual(180, mask_state.beam_stop_arm_angle)
        self.assertEqual(10, mask_state.beam_stop_arm_width)

    def test_beamstop_masking_x_y_positions(self):
        for beamstop_values in [{"width": 0.1, "angle": 0.2}, {"width": 0.1, "angle": 0.2, "x_pos": 0.3, "y_pos": 0.4}]:
            parser = self._setup_parser({"mask": {"spatial": {"beamstop_shadow": beamstop_values}}})
            masks = parser.get_state_mask(None)
            self.assertEqual(beamstop_values["width"], masks.beam_stop_arm_width)
            self.assertEqual(beamstop_values["angle"], masks.beam_stop_arm_angle)
            self.assertEqual(beamstop_values.get("x_pos", 0.0), masks.beam_stop_arm_pos1)
            self.assertEqual(beamstop_values.get("y_pos", 0.0), masks.beam_stop_arm_pos2)

    def test_parse_mask(self):
        top_level_dict = {"mask": {"prompt_peak": {}, "mask_files": [], "time": {"tof": []}}, "phi": {}}

        top_level_dict["mask"]["prompt_peak"] = {"start": 101, "stop": 102}

        mask_files_mock = [mock.NonCallableMock()]
        top_level_dict["mask"]["mask_files"] = mask_files_mock
        time_dict = top_level_dict["mask"]["time"]

        time_dict["tof"].extend([{"start": 100, "stop": 200}, {"start": 300, "stop": 400}])

        top_level_dict["mask"]["phi"] = {"mirror": False, "start": -50, "stop": 50}

        parser_result = self._setup_parser(top_level_dict)
        masks = parser_result.get_state_mask(None)

        self.assertIsInstance(masks, StateMask)
        self.assertEqual(mask_files_mock, masks.mask_files)

        self.assertEqual([100, 300], masks.bin_mask_general_start)
        self.assertEqual([200, 400], masks.bin_mask_general_stop)

        self.assertEqual(False, masks.use_mask_phi_mirror)
        self.assertEqual(-50, masks.phi_min)
        self.assertEqual(50, masks.phi_max)

        # TODO split below into own test
        transmission_state = parser_result.get_state_calculate_transmission()
        self.assertEqual(101, transmission_state.prompt_peak_correction_min)
        self.assertEqual(102, transmission_state.prompt_peak_correction_max)
        self.assertTrue(transmission_state.prompt_peak_correction_enabled)

        norm_state = parser_result.get_state_normalize_to_monitor(None)
        self.assertEqual(101, norm_state.prompt_peak_correction_min)
        self.assertEqual(102, norm_state.prompt_peak_correction_max)
        self.assertTrue(norm_state.prompt_peak_correction_enabled)


if __name__ == "__main__":
    unittest.main()
