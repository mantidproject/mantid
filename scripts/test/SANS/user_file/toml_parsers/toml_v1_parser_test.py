# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.common.enums import SANSInstrument, SANSFacility, DetectorType, ReductionMode, RangeStepType, FitModeForMerge, \
    DataType, FitType
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateMaskDetectors import StateMaskDetectors, StateMask
from sans.test_helper.file_information_mock import SANSFileInformationMock
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

    def _setup_parser(self, dict_vals):
        self._mocked_data_info = self._get_mock_data_info()
        if "instrument" not in dict_vals:
            # instrument key needs to generally be present
            dict_vals["instrument"] = {}
        if "name" not in dict_vals["instrument"]:
            dict_vals["instrument"]["name"] = "LOQ"

        return TomlV1Parser(dict_vals, data_info=self._mocked_data_info)

    def test_instrument(self):
        parser = self._setup_parser(dict_vals={"instrument": {"name": SANSInstrument.SANS2D.value}})
        inst = parser._implementation.instrument
        self.assertTrue(inst is SANSInstrument.SANS2D, msg="Got %r instead" % inst)

    def test_validate_is_called_on_init(self):
        schema_validator = mock.Mock()

        # No implementation needed
        with mock.patch("sans.user_file.toml_parsers.toml_v1_parser.TomlV1Parser._get_impl"):
            TomlV1Parser(dict_to_parse=None, data_info=mock.NonCallableMock(), schema_validator=schema_validator)
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
            ("norm_monitor", lambda x: x.get_state_calculate_transmission().incident_monitor),
            ("trans_monitor", lambda x: x.get_state_calculate_transmission().transmission_monitor),
            ("sample_aperture_diameter", lambda x: x.get_state_convert_to_q().q_resolution_a2),
            ("sample_offset", lambda x: x.get_state_move().sample_offset)
        ]

        self._loop_over_supported_keys(supported_keys=supported_keys, top_level_keys=["instrument", "configuration"])

    def test_detector_configuration_parsed(self):
        supported_keys = [
            ("rear_scale", lambda x: x.get_state_scale().scale),
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
        state_move = parser.get_state_move()

        self.assertTrue(parser.get_state_reduction_mode().reduction_mode is expected_reduction_mode)
        self.assertEqual((1, 2), get_beam_position(state_move, DetectorType.HAB))
        self.assertEqual((2, 3), get_beam_position(state_move, DetectorType.LAB))

    def test_binning_commands_parsed(self):
        # Wavelength
        for bin_type in ["Lin", "Log"]:
            wavelength_dict = {"binning": {"wavelength": {"start": 1.1, "step": 0.1, "stop": 2.2, "type": bin_type}}}
            wavelength = self._setup_parser(wavelength_dict).get_state_wavelength()
            self.assertEqual(1.1, wavelength.wavelength_low)
            self.assertEqual(0.1, wavelength.wavelength_step)
            self.assertEqual(2.2, wavelength.wavelength_high)
            self.assertEqual(RangeStepType(bin_type), wavelength.wavelength_step_type)

        one_d_reduction_q_dict = {"binning": {"1d_reduction": {"binning": "1.0, 0.1, 2.0, -0.2, 3.0"}}}
        one_d_convert_to_q = self._setup_parser(one_d_reduction_q_dict).get_state_convert_to_q()
        self.assertEqual(1.0, one_d_convert_to_q.q_min)
        self.assertEqual(3.0, one_d_convert_to_q.q_max)
        self.assertEqual("0.1, 2.0, -0.2", one_d_convert_to_q.q_1d_rebin_string.strip())

        two_d_reduction_q_dict = {"binning": {"2d_reduction": {"step": 1.0, "stop": 5.0, "type": "Lin"}}}
        two_d_convert_to_q = self._setup_parser(two_d_reduction_q_dict).get_state_convert_to_q()
        self.assertEqual(5.0, two_d_convert_to_q.q_xy_max)
        self.assertEqual(1.0, two_d_convert_to_q.q_xy_step)
        self.assertTrue(two_d_convert_to_q.q_xy_step_type is RangeStepType.LIN)

    def test_reduction_commands_parsed(self):
        top_level_dict = {"reduction": {"merged": {},
                                        "events": {}}}

        merged_dict = top_level_dict["reduction"]["merged"]
        merged_dict["merge_range"] = {"min": 1, "max": 2, "use_fit": False}
        merged_dict["rescale"] = {"min": 0.1, "max": 0.2, "use_fit": True}
        merged_dict["shift"] = {"min": 0.3, "max": 0.4, "use_fit": True}

        events_dict = top_level_dict["reduction"]["events"]

        expected_binning = "expected_binning"
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

    def test_detector_calibration(self):
        top_level_dict = {"detector": {"calibration": {"direct": {},
                                                       "flat": {},
                                                       "tube": {},
                                                       "position": {}}}}

        calibration_dict = top_level_dict["detector"]["calibration"]

        direct_front = mock.NonCallableMock()
        direct_rear = mock.NonCallableMock()
        calibration_dict["direct"]["front_file"] = direct_front
        calibration_dict["direct"]["rear_file"] = direct_rear

        flat_front = mock.NonCallableMock()
        flat_rear = mock.NonCallableMock()
        calibration_dict["flat"]["front_file"] = flat_front
        calibration_dict["flat"]["rear_file"] = flat_rear

        tube_file = mock.NonCallableMock()
        calibration_dict["tube"]["file"] = tube_file

        parser = self._setup_parser(top_level_dict)
        wavelength_state = parser.get_state_wavelength_and_pixel_adjustment()

        # Where Front = HAB and Rear = LAB in SANS terminology
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.LAB.value].wavelength_adjustment_file,
                         direct_rear)
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.HAB.value].wavelength_adjustment_file,
                         direct_front)

        self.assertEqual(wavelength_state.adjustment_files[DetectorType.LAB.value].pixel_adjustment_file,
                         flat_rear)
        self.assertEqual(wavelength_state.adjustment_files[DetectorType.HAB.value].pixel_adjustment_file,
                         flat_front)

        self.assertEqual(parser.get_state_adjustment().calibration, tube_file)

    def test_detector_calibration_position(self):
        top_level_dict = {"detector": {"calibration": {"position": {}}}}
        position_dict = top_level_dict["detector"]["calibration"]["position"]

        for adjustment in ["_x", "_y", "_z", "_rot", "_radius", "_side", "_x_tilt", "_y_tilt", "_z_tilt"]:
            position_dict["front" + adjustment] = mock.NonCallableMock()
            position_dict["rear" + adjustment] = mock.NonCallableMock()

        def assert_lab_hab_val(move_state, adjustment_name, state_name):
            lab_val = getattr(move_state.detectors[DetectorType.LAB.value], state_name)
            hab_val = getattr(move_state.detectors[DetectorType.HAB.value], state_name)
            self.assertEqual(position_dict["front" + adjustment_name], hab_val)
            self.assertEqual(position_dict["rear" + adjustment_name], lab_val)

        state_move = self._setup_parser(top_level_dict).get_state_move()
        assert_lab_hab_val(state_move, "_x", "x_translation_correction")
        assert_lab_hab_val(state_move, "_y", "y_translation_correction")

    def test_q_resolution(self):
        top_level_dict = {"q_resolution": {}}
        q_resolution_dict = top_level_dict["q_resolution"]

        q_resolution_dict["enabled"] = True
        q_resolution_dict["moderator_file"] = mock.NonCallableMock()
        q_resolution_dict["source_aperture"] = 1
        q_resolution_dict["delta_r"] = 2

        q_resolution = self._setup_parser(top_level_dict).get_state_convert_to_q()

        self.assertEqual(True, q_resolution.use_q_resolution)
        self.assertEqual(1, q_resolution.q_resolution_a1)
        self.assertEqual(2, q_resolution.q_resolution_delta_r)
        self.assertEqual(q_resolution_dict["moderator_file"], q_resolution.moderator_file)

    def test_gravity(self):
        test_dict = {"gravity": {"enabled": True}}
        q_state = self._setup_parser(test_dict).get_state_convert_to_q()

        self.assertEqual(True, q_state.use_gravity)

    def test_transmission(self):
        top_level_dict = {"transmission": {"monitor": {"M3": {}, "M5": {}}}}
        monitor_dict = top_level_dict["transmission"]["monitor"]

        m3_dict = monitor_dict["M3"]
        m3_dict["background"] = [100, 200]
        m3_dict["spectrum_number"] = 3
        m3_dict["shift"] = 10
        m3_dict["use_own_background"] = True

        m5_dict = monitor_dict["M5"]
        m5_dict["spectrum_number"] = 5
        m5_dict["use_own_background"] = False
        m5_dict["shift"] = -10

        top_level_dict["transmission"]["selected_monitor"] = "M3"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(10, parser.get_state_move().monitor_4_offset)
        self.assertEqual({'3': 100}, calc_transmission.background_TOF_monitor_start)
        self.assertEqual({'3': 200}, calc_transmission.background_TOF_monitor_stop)

        # Check switching the selected monitor picks up correctly
        top_level_dict["transmission"]["selected_monitor"] = "M5"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual(-10, parser.get_state_move().monitor_5_offset)
        self.assertFalse(calc_transmission.background_TOF_monitor_start)
        self.assertFalse(calc_transmission.background_TOF_monitor_stop)

        with self.assertRaises(KeyError):
            top_level_dict["transmission"]["selected_monitor"] = "M999"
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
        check_can_and_sample(parser, "wavelength_low", None)
        check_can_and_sample(parser, "wavelength_high", None)

        fitting_dict["function"] = "NotSet"
        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

        fitting_dict["enabled"] = False
        self.assertIsNotNone(self._setup_parser(top_level_dict))

    def test_parse_normalisation(self):
        # A2 is intentional to check were not hardcoded to Mx
        top_level_dict = {"normalisation": {"monitor": {"M1": {}, "A2": {}},
                                            "selected_monitor": ""}}
        monitor_dict = top_level_dict["normalisation"]["monitor"]

        m1_dict = monitor_dict["M1"]
        m1_dict["background"] = [100, 200]
        m1_dict["spectrum_number"] = 1

        a2_dict = monitor_dict["A2"]
        a2_dict["background"] = [400, 800]
        a2_dict["spectrum_number"] = 2

        top_level_dict["normalisation"]["selected_monitor"] = "M1"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual({'1': 100}, calc_transmission.background_TOF_monitor_start)
        self.assertEqual({'1': 200}, calc_transmission.background_TOF_monitor_stop)

        top_level_dict["normalisation"]["selected_monitor"] = "A2"
        parser = self._setup_parser(top_level_dict)
        calc_transmission = parser.get_state_calculate_transmission()
        self.assertEqual({'2': 400}, calc_transmission.background_TOF_monitor_start)
        self.assertEqual({'2': 800}, calc_transmission.background_TOF_monitor_stop)

        top_level_dict["normalisation"]["selected_monitor"] = "NotThere"
        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

        del a2_dict["background"]
        top_level_dict["normalisation"]["selected_monitor"] = "A2"
        with self.assertRaises(KeyError):
            self._setup_parser(top_level_dict)

    def test_parse_mask_spatial(self):
        top_level_dict = {"mask": {"spatial": {"rear": {},
                                               "front": {}}}}

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

        mask_state = self._setup_parser(top_level_dict).get_state_mask()

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

    def test_parse_mask(self):
        top_level_dict = {"mask": {"beamstop_shadow": {},
                                   "mask_pixels": [],
                                   "mask_files": [],
                                   "time": {"tof": []}}}

        top_level_dict["mask"]["beamstop_shadow"] = {"width": 10, "angle": 180}

        mask_files_mock = [mock.NonCallableMock()]
        mask_pixels_expected = [1, 2, 4, 17000]  # 17000 is in HAB on LOQ
        top_level_dict["mask"]["mask_files"] = mask_files_mock
        top_level_dict["mask"]["mask_pixels"] = mask_pixels_expected

        time_dict = top_level_dict["mask"]["time"]

        time_dict["tof"].extend([{"start": 100, "stop": 200},
                                 {"start": 300, "stop": 400}])

        masks = self._setup_parser(top_level_dict).get_state_mask()

        self.assertIsInstance(masks, StateMask)
        self.assertEqual(180, masks.beam_stop_arm_angle)
        self.assertEqual(10, masks.beam_stop_arm_width)

        self.assertEqual(mask_files_mock, masks.mask_files)
        self.assertEqual([1, 2, 4], masks.detectors[DetectorType.LAB.value].single_spectra)
        self.assertEqual([17000], masks.detectors[DetectorType.HAB.value].single_spectra)

        self.assertEqual([100, 300], masks.bin_mask_general_start)
        self.assertEqual([200, 400], masks.bin_mask_general_stop)


if __name__ == '__main__':
    unittest.main()
