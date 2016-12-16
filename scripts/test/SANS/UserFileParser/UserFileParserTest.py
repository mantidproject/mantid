import unittest
import mantid

from SANS2.UserFile.UserFileParser import *


# -----------------------------------------------------------------
# --- Free Helper Functions for Testing ---------------------------
# -----------------------------------------------------------------
def assert_valid_result(result, expected, assert_true):
    keys_result = result.keys()
    keys_expected = expected.keys()
    assert_true(len(keys_expected) == len(keys_result))
    for key in keys_result:
        assert_true(key in keys_expected)
        assert_true(result[key] == expected[key])


def assert_valid_parse(parser, to_parse, expected, assert_true):
    result = parser.parse_line(to_parse)
    # Same amount of keys
    assert_valid_result(result, expected, assert_true)


def assert_invalid_parse(parser, to_parse, exception, assert_raises):
    assert_raises(exception, parser.parse_line, to_parse)


def do_test(parser, valid_settings, invalid_settings, assert_true, assert_raises):
    for setting in valid_settings:
        assert_valid_parse(parser, setting, valid_settings[setting], assert_true)

    for setting in invalid_settings:
        assert_invalid_parse(parser, setting, invalid_settings[setting], assert_raises)


# -----------------------------------------------------------------
# --- Tests -------------------------------------------------------
# -----------------------------------------------------------------
class DetParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(DetParser.get_type(), "DET")

    def test_that_reduction_mode_is_parsed_correctly(self):
        # The dict below has the string to parse as the key and the expected result as a value
        valid_settings = {"DET/HAB": {user_file_reduction_mode: "HAB"},
                          "dEt/ frONT ": {user_file_reduction_mode: "HAB"},
                          "dET/REAR": {user_file_reduction_mode: "LAB"},
                          "dEt/MAIn   ": {user_file_reduction_mode: "LAB"},
                          " dEt/ BOtH": {user_file_reduction_mode: "BOTH"},
                          "DeT /merge ": {user_file_reduction_mode: "MERGE"},
                          " DEt / MERGED": {user_file_reduction_mode: "MERGE"}}

        invalid_settings = {"DET/HUB": RuntimeError,
                            "DET/HAB/": RuntimeError}
        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_merge_option_is_parsed_correctly(self):
        valid_settings = {"DET/RESCALE 123": {user_file_rescale: 123},
                          "dEt/ shiFt 48.5": {user_file_shift: 48.5},
                          "dET/reSCale/FIT   23 34.6 ": {user_file_rescale_shift_fit_min: 23,
                                                         user_file_rescale_shift_fit_max: 34.6},
                          "dEt/SHIFT/FIT 235.2  341   ": {user_file_rescale_shift_fit_min: 235.2,
                                                          user_file_rescale_shift_fit_max: 341}}

        invalid_settings = {"DET/Ruscale": RuntimeError,
                            "DET/SHIFT/": RuntimeError,
                            "DET/SHIFT 1 2": RuntimeError,
                            "DET/SHIFT/FIT 1 ": RuntimeError,
                            "DET/Rescale/FIT 1 2 4": RuntimeError}

        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_detector_setting_is_parsed_correctly(self):
        valid_settings = {"Det/CORR/REAR/X 123": {user_file_correction_x_lab: 123},
                          "DEt/CORR/ frOnt/X +95.7": {user_file_correction_x_hab: 95.7},
                          "DeT/ CORR / ReAR/ y 12.3": {user_file_correction_y_lab: 12.3},
                          " DET/CoRR/fROnt/Y -957": {user_file_correction_y_hab: -957},
                          "DeT/ CORR /reAR/Z 12.3": {user_file_correction_z_lab: 12.3},
                          " DET/CoRR/FRONT/ Z -957": {user_file_correction_z_hab: -957},
                          "DeT/ CORR /reAR/SIDE 12.3": {user_file_correction_translation_lab: 12.3},
                          " DET/CoRR/FRONT/ SidE -957": {user_file_correction_translation_hab: -957},
                          "DeT/ CORR /reAR/ROt 12.3": {user_file_correction_rotation_lab: 12.3},
                          " DET/CoRR/FRONT/ROT -957": {user_file_correction_rotation_hab: -957},
                          "DeT/ CORR /reAR/Radius 12.3": {user_file_correction_radius_lab: 12.3},
                          " DET/CoRR/FRONT/RADIUS 957": {user_file_correction_radius_hab: 957}}

        invalid_settings = {"Det/CORR/REAR/X ": RuntimeError,
                            "DEt/CORR/ frOnt/X 12 23": RuntimeError,
                            " DET/CoRR/fROnt": RuntimeError,
                            "DeT/ CORR /reAR/Z test": RuntimeError,
                            " DET/CoRR/FRONT/ ZZ -957": RuntimeError,
                            "DeT/ CORR /reAR/SIDE D 12.3": RuntimeError,
                            " DET/CoRR/FRONT/ SidE -i3": RuntimeError}

        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class LimitParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(LimitParser.get_type(), "L")

    def test_that_angle_limit_is_parsed_correctly(self):
        valid_settings = {"L/PhI 123   345.2": {user_file_limit_angle_min: 123,
                                              user_file_limit_angle_max: 345.2,
                                              user_file_no_mirror: False},
                          "L/PHI / NOMIRROR 123 -345.2": {user_file_limit_angle_min: 123,
                                                       user_file_limit_angle_max: -345.2,
                                                        user_file_no_mirror: True},
                          }

        invalid_settings = {"L/PHI/NMIRROR/ 23 454": RuntimeError,
                            "L /pHI/ 23": RuntimeError,
                            "L/PhI/ f f": RuntimeError,
                            "L/ PHI/ f f": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_event_time_limit_is_parsed_correctly(self):
        valid_settings = {"L  / EVEnTStime 0,-10,32,434,34523,35": {user_file_events_binning: [0,-10,32,434,34523,35]}
                          }

        invalid_settings = {"L  / EEnTStime 0,-10,32,434,34523,35": RuntimeError,
                            "L/EVENTSTIME 123g, sdf": RuntimeError,
                            "L  /EvEnTStime": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_cut_limits_are_parsed_correctly(self):
        valid_settings = {"L/Q/RCUT 234.4": {user_file_radius_cut: 234.4},
                          "L /q / RcUT -234.34": {user_file_radius_cut: -234.34},
                          "l/Q/WCUT 234.4": {user_file_wavelength_cut: 234.4},
                          "L /q / wcUT -234.34": {user_file_wavelength_cut: -234.34}}

        invalid_settings = {"L/Q/Rcu 123": RuntimeError,
                            "L/Q/RCUT/ 2134": RuntimeError,
                            "L/Q/Wcut 23 234": RuntimeError,
                            "L/Q/WCUT": RuntimeError,
                            "L / Q / WCUT234": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_radius_limits_are_parsed_correctly(self):
        valid_settings = {"L/R 234 235": {user_file_radius_limit_min: 234,
                                          user_file_radius_limit_max: 235},
                          "L / r   -234   235": {user_file_radius_limit_min: -234,
                                          user_file_radius_limit_max: 235}
                          }
        invalid_settings = {"L/R/ 234 435": RuntimeError,
                            "L/Rr 234 435": RuntimeError,
                            "L/R 435": RuntimeError,
                            "L/R 234 324 435": RuntimeError,
                            "L/R sdf": RuntimeError,
                            "L/R": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_q_limits_are_parsed_correctly(self):
        valid_settings = {"L/Q 12 34": {user_file_q_min: 12, user_file_q_max: 34},
                          "L/Q 12 34 2.7": {user_file_q_min: 12, user_file_q_max: 34, user_file_q_step: 2.7,
                                            user_file_q_step_type: "LIN"},
                          "L/Q -12 34.6 2.7/LOG": {user_file_q_min: -12, user_file_q_max: 34.6, user_file_q_step: 2.7,
                                                   user_file_q_step_type: "LOG"},
                          "L/q -12 3.6 2 /LIN": {user_file_q_min: -12, user_file_q_max: 3.6, user_file_q_step: 2,
                                                 user_file_q_step_type: "LIN"},
                          "L/q -12   0.4  23 34.8 3.6": {user_file_q_min: -12, user_file_q_step_min_mid: 0.4,
                                                         user_file_q_mid: 23, user_file_q_step_mid_max: 34.8,
                                                         user_file_q_max: 3.6, user_file_q_step_type: "LIN"},
                          "L/q -12   0.4  23 34.8 3.6 /LIn": {user_file_q_min: -12, user_file_q_step_min_mid: 0.4,
                                                              user_file_q_mid: 23, user_file_q_step_mid_max: 34.8,
                                                              user_file_q_max: 3.6, user_file_q_step_type: "LIN"},
                          "L/q -12   0.4  23  34.8 3.6  /Log": {user_file_q_min: -12, user_file_q_step_min_mid: 0.4,
                                                                user_file_q_mid: 23, user_file_q_step_mid_max: 34.8,
                                                                user_file_q_max: 3.6, user_file_q_step_type: "LOG"}}

        invalid_settings = {"L/Q 12 2 3 4": RuntimeError,
                            "L/Q 12 2 3 4 23 3": RuntimeError,
                            "L/Q 12 2 3 4 5/LUG": RuntimeError,
                            "L/Q 12 2 /LIN": RuntimeError,
                            "L/Q ": RuntimeError,
                            "L/Q a 1 2 3 4 /LIN": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


    def test_that_qxy_limits_are_parsed_correctly(self):
        valid_settings = {"L/QXY 12 34": {user_file_qxy_min: 12, user_file_qxy_max: 34},
                          "L/QXY 12 34 2.7": {user_file_qxy_min: 12, user_file_qxy_max: 34, user_file_qxy_step: 2.7,
                                            user_file_qxy_step_type: "LIN"},
                          "L/QXY -12 34.6 2.7/LOG": {user_file_qxy_min: -12, user_file_qxy_max: 34.6,
                                                     user_file_qxy_step: 2.7, user_file_qxy_step_type: "LOG"},
                          "L/qxY -12 3.6 2 /LIN": {user_file_qxy_min: -12, user_file_qxy_max: 3.6,
                                                   user_file_qxy_step: 2, user_file_qxy_step_type: "LIN"},
                          "L/qxy -12   0.4  23 34.8 3.6": {user_file_qxy_min: -12, user_file_qxy_step_min_mid: 0.4,
                                                         user_file_qxy_mid: 23, user_file_qxy_step_mid_max: 34.8,
                                                         user_file_qxy_max: 3.6, user_file_qxy_step_type: "LIN"},
                          "L/qXY -12   0.4  23 34.8 3.6 /LIn": {user_file_qxy_min: -12, user_file_qxy_step_min_mid: 0.4,
                                                              user_file_qxy_mid: 23, user_file_qxy_step_mid_max: 34.8,
                                                              user_file_qxy_max: 3.6, user_file_qxy_step_type: "LIN"},
                          "L/qXY -12   0.4  23  34.8 3.6  /Log": {user_file_qxy_min: -12,
                                                                  user_file_qxy_step_min_mid: 0.4,
                                                                  user_file_qxy_mid: 23,
                                                                  user_file_qxy_step_mid_max: 34.8,
                                                                  user_file_qxy_max: 3.6,
                                                                  user_file_qxy_step_type: "LOG"}}

        invalid_settings = {"L/QXY 12 2 3 4": RuntimeError,
                            "L/QXY 12 2 3 4 23 3": RuntimeError,
                            "L/QXY 12 2 3 4 5/LUG": RuntimeError,
                            "L/QXY 12 2 /LIN": RuntimeError,
                            "L/QXY ": RuntimeError,
                            "L/QXY a 1 2 3 4 /LIN": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_wavelength_limits_are_parsed_correctly(self):
        valid_settings = {"L/WAV 12 34": {user_file_wavelength_min: 12, user_file_wavelength_max: 34},
                          "L/waV 12 34 2.7": {user_file_wavelength_min: 12, user_file_wavelength_max: 34,
                                              user_file_wavelength_step: 2.7, user_file_wavelength_step_type: "LIN"},
                          "L/wAv -12 34.6 2.7/LOG": {user_file_wavelength_min: -12,  user_file_wavelength_max: 34.6,
                                                     user_file_wavelength_step: 2.7,
                                                     user_file_wavelength_step_type: "LOG"},
                          "L/WaV -12 3.6 2 /LIN": {user_file_wavelength_min: -12, user_file_wavelength_max: 3.6,
                                                   user_file_wavelength_step: 2, user_file_wavelength_step_type: "LIN"},
                          "L/wav -12   0.4  23 34.8 3.6": {user_file_wavelength_min: -12,
                                                           user_file_wavelength_step_min_mid: 0.4,
                                                           user_file_wavelength_mid: 23,
                                                           user_file_wavelength_step_mid_max: 34.8,
                                                           user_file_wavelength_max: 3.6,
                                                           user_file_wavelength_step_type: "LIN"},
                          "L/wav -12   0.4  23 34.8 3.6 /LIn": {user_file_wavelength_min: -12,
                                                                user_file_wavelength_step_min_mid: 0.4,
                                                                user_file_wavelength_mid: 23,
                                                                user_file_wavelength_step_mid_max: 34.8,
                                                                user_file_wavelength_max: 3.6,
                                                                user_file_wavelength_step_type: "LIN"},
                          "L/wav -12   0.4  23  34.8 3.6  /Log": {user_file_wavelength_min: -12,
                                                                  user_file_wavelength_step_min_mid: 0.4,
                                                                  user_file_wavelength_mid: 23,
                                                                  user_file_wavelength_step_mid_max: 34.8,
                                                                  user_file_wavelength_max: 3.6,
                                                                  user_file_wavelength_step_type: "LOG"}}

        invalid_settings = {"L/WAV 12 2 3 4": RuntimeError,
                            "L/WAV 12 2 3 4 23 3": RuntimeError,
                            "L/WAV 12 2 3 4 5/LUG": RuntimeError,
                            "L/WAV 12 2 /LIN": RuntimeError,
                            "L/WAV ": RuntimeError,
                            "L/WAV a 1 2 3 4 /LIN": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class MaskParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(MaskParser.get_type(), "MASK")

    def test_that_masked_line_is_parsed_correctly(self):
        valid_settings = {"MASK/LiNE 12  23.6": {user_file_line_width: 12,
                                                 user_file_line_angle: 23.6},
                          "MASK/LiNE 12  23.6 2 346": {user_file_line_width: 12,
                                                       user_file_line_angle: 23.6,
                                                       user_file_line_x: 2,
                                                       user_file_line_y: 346},
                          "MASK/TIME 23 35": {user_file_time_min: 23,
                                              user_file_time_max: 35},
                          "MASK/T 23 35": {user_file_time_min: 23,
                                          user_file_time_max: 35}
                          }
        invalid_settings = {"MASK/LiN 12 4": RuntimeError,
                            "MASK/LINE 12": RuntimeError,
                            "MASK/LINE 12 34 345 6 7": RuntimeError,
                            "MASK/LINE ": RuntimeError,
                            "MASK/LINE  x y": RuntimeError,
                            "MASK/TIME 12 34 4 ": RuntimeError,
                            "MASK/T 2": RuntimeError,
                            }

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_masked_time_is_parsed_correctly(self):
        valid_settings = {"MASK/TIME 23 35": {user_file_time_min: 23,
                                              user_file_time_max: 35},
                          "MASK/T 23 35": {user_file_time_min: 23,
                                           user_file_time_max: 35}
                          }

        invalid_settings = {"MASK/TIME 12 34 4 ": RuntimeError,
                            "MASK/T 2": RuntimeError,
                            "MASK/T": RuntimeError,
                            "MASK/T x y": RuntimeError}

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_clear_mask_is_parsed_correctly(self):
        valid_settings = {"MASK/CLEAR": {user_file_clear_detector_mask: True},
                          "MASK/CLeaR /TIMe": {user_file_clear_time_mask: True}}

        invalid_settings = {"MASK/CLEAR/TIME/test": RuntimeError,
                            "MASK/CLEAR/TIIE": RuntimeError,
                            "MASK/CLEAR test": RuntimeError}

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_spectrum_mask_is_parsed_correctly(self):
        valid_settings = {"MASK S 12  ": {use_file_single_spectrum_mask: 12},
                          "MASK S234": {use_file_single_spectrum_mask: 234}}

        invalid_settings = {"MASK B 12  ": RuntimeError,
                            "MASK S 12 23 ": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_spectrum_mask_is_parsed_correctly(self):
        valid_settings = {"MASK S 12 >  S23  ": {use_file_spectrum_range_mask: [12, 23]},
                          "MASK S234>S1234": {use_file_spectrum_range_mask: [234, 1234]}}

        invalid_settings = {"MASK S 12> S123.5  ": RuntimeError,
                            "MASK S 12> 23 ": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_vertical_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK V 12  ": {user_file_vertical_single_strip_mask_lab: 12},
                          "MASK / Rear V  12  ": {user_file_vertical_single_strip_mask_lab: 12},
                          "MASK/mAin V234": {user_file_vertical_single_strip_mask_lab: 234},
                          "MASK / LaB V  234": {user_file_vertical_single_strip_mask_lab: 234},
                          "MASK /frOnt V  12  ": {user_file_vertical_single_strip_mask_hab: 12},
                          "MASK/HAB V234": {user_file_vertical_single_strip_mask_hab: 234}}

        invalid_settings = {"MASK B 12  ": RuntimeError,
                            "MASK V 12 23 ": RuntimeError,
                            "MASK \Rear V3": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_range_vertical_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK V  12 >  V23  ": {user_file_vertical_range_strip_mask_lab: [12, 23]},
                          "MASK V123>V234": {user_file_vertical_range_strip_mask_lab: [123, 234]},
                          "MASK / Rear V123>V234": {user_file_vertical_range_strip_mask_lab: [123, 234]},
                          "MASK/mAin  V123>V234": {user_file_vertical_range_strip_mask_lab: [123, 234]},
                          "MASK / LaB V123>V234": {user_file_vertical_range_strip_mask_lab: [123, 234]},
                          "MASK/frOnt V123>V234": {user_file_vertical_range_strip_mask_hab: [123, 234]},
                          "MASK/HAB V123>V234": {user_file_vertical_range_strip_mask_hab: [123, 234]}}

        invalid_settings = {"MASK V 12> V123.5  ": RuntimeError,
                            "MASK V 12 23 ": RuntimeError,
                            "MASK /Rear/ V12>V34": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_horizontal_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK H 12  ": {user_file_horizontal_single_strip_mask_lab: 12},
                          "MASK / Rear H  12  ": {user_file_horizontal_single_strip_mask_lab: 12},
                          "MASK/mAin H234": {user_file_horizontal_single_strip_mask_lab: 234},
                          "MASK / LaB H  234": {user_file_horizontal_single_strip_mask_lab: 234},
                          "MASK /frOnt H  12  ": {user_file_horizontal_single_strip_mask_hab: 12},
                          "MASK/HAB H234": {user_file_horizontal_single_strip_mask_hab: 234}}

        invalid_settings = {"MASK H/12  ": RuntimeError,
                            "MASK H 12 23 ": RuntimeError,
                            "MASK \Rear H3": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_range_horizontal_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK H  12 >  H23  ": {user_file_horizontal_range_strip_mask_lab: [12, 23]},
                          "MASK H123>H234": {user_file_horizontal_range_strip_mask_lab: [123, 234]},
                          "MASK / Rear H123>H234": {user_file_horizontal_range_strip_mask_lab: [123, 234]},
                          "MASK/mAin  H123>H234": {user_file_horizontal_range_strip_mask_lab: [123, 234]},
                          "MASK / LaB H123>H234": {user_file_horizontal_range_strip_mask_lab: [123, 234]},
                          "MASK/frOnt H123>H234": {user_file_horizontal_range_strip_mask_hab: [123, 234]},
                          "MASK/HAB H123>H234": {user_file_horizontal_range_strip_mask_hab: [123, 234]}}

        invalid_settings = {"MASK H 12> H123.5  ": RuntimeError,
                            "MASK H 12 23 ": RuntimeError,
                            "MASK /Rear/ H12>V34": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SampleParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SampleParser.get_type(), "SAMPLE")

    def test_that_setting_sample_path_is_parsed_correctly(self):
        valid_settings = {"SAMPLE /PATH/ON" : {user_file_sample_path: True},
                          "SAMPLE / PATH / OfF": {user_file_sample_path: False}}

        invalid_settings = {"SAMPLE/PATH ON": RuntimeError,
                            "SAMPLE /pATh ": RuntimeError,
                            "SAMPLE/ Path ONN": RuntimeError}

        sample_parser = SampleParser()
        do_test(sample_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_setting_sample_offset_is_parsed_correctly(self):
        valid_settings = {"SAMPLE /Offset 234.5": {user_file_sample_offset: 234.5},
                          "SAMPLE / Offset 25": {user_file_sample_offset: 25}}

        invalid_settings = {"SAMPL/offset fg": RuntimeError,
                            "SAMPLE /Offset/ 23 ": RuntimeError,
                            "SAMPLE/ offset 234 34": RuntimeError}

        sample_parser = SampleParser()
        do_test(sample_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SetParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SetParser.get_type(), "SET")

    def test_that_setting_scales_is_parsed_correctly(self):
        valid_settings = {"SET  scales 2 5 4    7 8": {user_file_scales: [2, 5, 4, 7, 8]}}

        invalid_settings = {"SET scales 2 4 6 7 8 9": RuntimeError,
                            "SET scales ": RuntimeError}

        set_parser = SetParser()
        do_test(set_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_centre_is_parsed_correctly(self):
        valid_settings = {"SET centre 23 45": {user_file_centre_lab: [23, 45]},
                          "SET centre /main 23 45": {user_file_centre_lab: [23, 45]},
                          "SET centre / lAb 23 45": {user_file_centre_lab: [23, 45]},
                          "SET centre / hAb 23 45": {user_file_centre_hab: [23, 45]},
                          "SET centre /FRONT 23 45": {user_file_centre_hab: [23, 45]}}

        invalid_settings = {"SET centre 23": RuntimeError,
                            "SEt centre 34 34 34": RuntimeError,
                            "SEt centre/MAIN/ 34 34": RuntimeError,
                            "SEt centre/MAIN": RuntimeError}

        set_parser = SetParser()
        do_test(set_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class TransParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(TransParser.get_type(), "TRANS")

    def test_that_trans_spec_is_parsed_correctly(self):
        valid_settings = {"TRANS/TRANSSPEC=23": {user_file_trans_spec: 23},
                          "TRANS / TransSPEC =  23": {user_file_trans_spec: 23}}

        invalid_settings = {"TRANS/TRANSSPEC 23": RuntimeError,
                            "TRANS/TRANSSPEC/23": RuntimeError,
                            "TRANS/TRANSSPEC=23.5": RuntimeError,
                            "TRANS/TRANSSPEC=2t": RuntimeError,
                            "TRANS/TRANSPEC=23": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_trans_spec_shift_is_parsed_correctly(self):
        valid_settings = {"TRANS/TRANSSPEC=4/SHIFT=23": {user_file_trans_spec_shift: 23},
                          "TRANS/TRANSSPEC =4/ SHIFT = 23": {user_file_trans_spec_shift: 23}}

        invalid_settings = {"TRANS/TRANSSPEC=6/SHIFT=23": RuntimeError,
                            "TRANS/TRANSSPEC=4/SHIFT/23": RuntimeError,
                            "TRANS/TRANSSPEC=4/SHIFT 23": RuntimeError,
                            "TRANS/TRANSSPEC/SHIFT=23": RuntimeError,
                            "TRANS/TRANSSPEC=6/SHIFT=t": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_radius_is_parsed_correctly(self):
        valid_settings = {"TRANS / radius  =23": {user_file_trans_radius: 23},
                          "TRANS /RADIUS= 245.7": {user_file_trans_radius: 245.7}}
        invalid_settings = {}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_roi_is_parsed_correctly(self):
        valid_settings = {"TRANS/ROI =testFile.xml": {user_file_trans_roi: ["testFile.xml"]},
                          "TRANS/ROI =testFile.xml, "
                          "TestFile2.XmL,testFile4.xml": {user_file_trans_roi: ["testFile.xml", "TestFile2.XmL",
                                                                                "testFile4.xml"]}}
        invalid_settings = {"TRANS/ROI =t estFile.xml": RuntimeError,
                            "TRANS/ROI =testFile.txt": RuntimeError,
                            "TRANS/ROI testFile.txt": RuntimeError,
                            "TRANS/ROI=": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_mask_is_parsed_correctly(self):
        valid_settings = {"TRANS/Mask =testFile.xml": {user_file_trans_mask: ["testFile.xml"]},
                          "TRANS/ MASK =testFile.xml, "
                          "TestFile2.XmL,testFile4.xml": {user_file_trans_mask: ["testFile.xml", "TestFile2.XmL",
                                                                                "testFile4.xml"]}}
        invalid_settings = {"TRANS/MASK =t estFile.xml": RuntimeError,
                            "TRANS/  MASK =testFile.txt": RuntimeError,
                            "TRANS/ MASK testFile.txt": RuntimeError,
                            "TRANS/MASK=": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_workspaces_are_parsed_correctly(self):
        valid_settings = {"TRANS/SampleWS =testworksaoe234Name":
                              {user_file_trans_sample_workspace: "testworksaoe234Name"},
                          "TRANS/ SampleWS = testworksaoe234Name":
                              {user_file_trans_sample_workspace: "testworksaoe234Name"},
                          "TRANS/ CanWS =testworksaoe234Name":
                              {user_file_trans_can_workspace: "testworksaoe234Name"},
                          "TRANS/ CANWS = testworksaoe234Name": {
                              user_file_trans_can_workspace: "testworksaoe234Name"}}
        invalid_settings = {"TRANS/CANWS/ test": RuntimeError,
                            "TRANS/CANWS/ test": RuntimeError,
                            "TRANS/SAMPLEWS =": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class TubeCalibFileParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(TubeCalibFileParser.get_type(), "TRANS")

    def test_that_tube_calibration_file_is_parsed_correctly(self):
        valid_settings = {"TUBECALIbfile= calib_file.nxs": {user_file_tube_calibration: "calib_file.nxs"},
                          " tUBECALIBfile=  caAlib_file.Nxs": {user_file_tube_calibration: "caAlib_file.Nxs"}}

        invalid_settings = {"TUBECALIFILE file.nxs": RuntimeError,
                            "TUBECALIBFILE=file.txt": RuntimeError,
                            "TUBECALIBFILE=file": RuntimeError}

        tube_calib_file_parser = TubeCalibFileParser()
        do_test(tube_calib_file_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class QResolutionParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(QResolutionParser.get_type(), "QRESOLUTION")

    def test_that_q_resolution_on_off_is_parsed_correctly(self):
        valid_settings = {"QRESOLUTION/ON": {user_file_q_resolution_on: True},
                          "QRESOLUTIoN / oFF": {user_file_q_resolution_on: False}}

        invalid_settings = {"QRESOLUTION= ON": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_q_resolution_float_values_are_parsed_correctly(self):
        valid_settings = {"QRESOLUTION/deltaR = 23.546": {user_file_q_resolution_delta_r: 23.546},
                          "QRESOLUTION/ Lcollim = 23.546": {user_file_q_resolution_collimation_length: 23.546},
                          "QRESOLUTION/ a1 = 23.546": {user_file_q_resolution_a1: 23.546},
                          "QRESOLUTION/ a2 =  23": {user_file_q_resolution_a2: 23},
                          "QREsolution /  H1 = 23.546 ": {user_file_q_resolution_h1: 23.546},
                          "QREsolution /h2 = 23.546 ": {user_file_q_resolution_h2: 23.546},
                          "QREsolution /  W1 = 23.546 ": {user_file_q_resolution_w1: 23.546},
                          "QREsolution /W2 = 23.546 ": {user_file_q_resolution_w2: 23.546}
                          }

        invalid_settings = {"QRESOLUTION/DELTAR 23": RuntimeError,
                            "QRESOLUTION/DELTAR = test": RuntimeError,
                            "QRESOLUTION/A1 t": RuntimeError,
                            "QRESOLUTION/B1=10": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_moderator_is_parsed_correctly(self):
        valid_settings = {"QRESOLUTION/MODERATOR = test_file.txt": {user_file_q_resolution_moderator: "test_file.txt"}}

        invalid_settings = {"QRESOLUTION/MODERATOR = test_file.nxs": RuntimeError,
                            "QRESOLUTION/MODERATOR/test_file.txt": RuntimeError,
                            "QRESOLUTION/MODERATOR=test_filetxt": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class FitParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(FitParser.get_type(), "FIT")

    def test_that_trans_clear_is_parsed_correctly(self):
        valid_settings = {"FIT/ trans / clear": {user_file_fit_clear: True},
                          "FIT/traNS /ofF": {user_file_fit_clear: True}}

        invalid_settings = {"FIT/  clear": RuntimeError,
                            "FIT/MONITOR/OFF": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_range_based_fit_is_parsed_correctly(self):
        valid_settings = {"FIT/ trans / LIN 123 3556": {user_file_range_based_fit_lin: [123, 3556]},
                          "FIT/ tranS/linear 123 3556": {user_file_range_based_fit_lin: [123, 3556]},
                          "FIT/TRANS/Straight 123 3556": {user_file_range_based_fit_lin: [123, 3556]},
                          "FIT/ tranS/LoG 123  3556.6 ": {user_file_range_based_fit_log: [123, 3556.6]},
                          "FIT/TRANS/  YlOG 123   3556": {user_file_range_based_fit_log: [123, 3556]}}

        invalid_settings = {"FIT/TRANS/ YlOG 123": RuntimeError,
                            "FIT/TRANS/ YlOG 123 34 34": RuntimeError,
                            "FIT/TRANS/ YlOG 123 fg": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_monitor_times_are_parsed_correctly(self):
        valid_settings = {"FIT/monitor 12 34.5": {user_file_monitor_times: [12, 34.5]},
                          "Fit / Monitor 12.6 34.5": {user_file_monitor_times: [12.6, 34.5]}}

        invalid_settings = {"Fit / Monitor 12.6 34 34": RuntimeError,
                            "Fit / Monitor": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_general_fit_is_parsed_correctly(self):
        valid_settings = {"FIT/Trans/Lin": {user_file_fit_can: user_file_lin,
                                            user_file_fit_sample: user_file_lin},
                          "FIT/Trans/ Log": {user_file_fit_can: user_file_log,
                                            user_file_fit_sample: user_file_log},
                          "FIT/Trans/ polYnomial": {user_file_fit_can_poly: 2,
                                             user_file_fit_sample_poly: 2},
                          "FIT/Trans/ polYnomial 3": {user_file_fit_can_poly: 3,
                                              user_file_fit_sample_poly: 3},
                          "FIT/Trans/Sample/Log": { user_file_fit_sample: user_file_log},
                          "FIT/Trans/Sample/ Lin": {user_file_fit_sample: user_file_lin},
                          "FIT/Trans / can/Log": {user_file_fit_can: user_file_log},
                          "FIT/ Trans/CAN/ lin": {user_file_fit_can: user_file_lin},
                          "FIT/Trans/Sample/ polynomiAL 4": {user_file_fit_sample_poly: 4},
                          "FIT/Trans / can/polynomiAL 5": {user_file_fit_can_poly: 5}}

        invalid_settings = {"FIT/Trans / can/polynomiAL 6": RuntimeError,
                            "FIT/Trans /": RuntimeError,
                            "FIT/Trans": RuntimeError,
                            "FIT/Trans / Lin 23": RuntimeError,
                            "FIT/Trans / lin 23 5 6": RuntimeError,
                            "FIT/Trans / lin 23 t": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class GravityParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(GravityParser.get_type(), "GRAVITY")

    def test_that_gravity_on_off_is_parsed_correctly(self):
        valid_settings = {"Gravity on ": {user_file_gravity_on: True},
                          "Gravity   OFF ": {user_file_gravity_on: False}}

        invalid_settings = {"Gravity ": RuntimeError,
                            "Gravity ONN": RuntimeError}

        gravity_parser = GravityParser()
        do_test(gravity_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_gravity_extra_length_is_parsed_correctly(self):
        valid_settings = {"Gravity/LExtra =23.5": {user_file_gravity_extra_length: 23.5},
                          "Gravity  / lExtra =  23.5": {user_file_gravity_extra_length: 23.5}}

        invalid_settings = {"Gravity/LExtra 23.5": RuntimeError,
                            "Gravity/LExtra =tw": RuntimeError}

        gravity_parser = GravityParser()
        do_test(gravity_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class MaskFileParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(MaskFileParser.get_type(), "MASKFILE")

    def test_that_gravity_on_off_is_parsed_correctly(self):
        valid_settings = {"MaskFile= test.xml,   testKsdk2.xml,tesetlskd.xml":
                          {user_file_mask_file: ["test.xml", "testKsdk2.xml", "tesetlskd.xml"]}}

        invalid_settings = {"MaskFile=": RuntimeError,
                            "MaskFile=test.txt": RuntimeError,
                            "MaskFile test.xml, test2.xml": RuntimeError}

        mask_file_parser = MaskFileParser()
        do_test(mask_file_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class MonParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(MonParser.get_type(), "MON")

    def test_that_length_is_parsed_correctly(self):
        valid_settings = {"MON/length= 23.5 34": {user_file_monitor_length: [23.5, 34]},
                          "MON/length= 23.5 34  / InterPolate": {user_file_monitor_length: [23.5, 34],
                                                                 user_file_mon_interpolate: True}}

        invalid_settings = {"MON/length= 23.5 34.7": RuntimeError,
                            "MON/length 23.5 34": RuntimeError,
                            "MON/length=23.5": RuntimeError,
                            "MON/length/23.5 34": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_direct_files_are_parsed_correctly(self):
        valid_settings = {"MON/DIRECT= C:\path1\Path2\file.ext ": {user_file_direct_lab: "C:/path1/Path2/file.ext"},
                          "MON/ direct  = filE.Ext ": {user_file_direct_lab: "filE.Ext"},
                          "MON/DIRECT= \path1\Path2\file.ext ": {user_file_direct_lab: "/path1/Path2/file.ext"},
                          "MON/DIRECT= /path1/Path2/file.ext ": {user_file_direct_lab: "/path1/Path2/file.ext"},
                          "MON/DIRECT= /path1/Path2/file.ext ": {user_file_direct_lab: "/path1/Path2/file.ext"},
                          "MON/DIRECT/ rear= /path1/Path2/file.ext ": {user_file_direct_lab: "/path1/Path2/file.ext"},
                          "MON/DIRECT/ frONT= path1/Path2/file.ext ": {user_file_direct_hab: "path1/Path2/file.ext"}}

        invalid_settings = {"MON/DIRECT= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/DIRECT /path1/Path2/file.ext ": RuntimeError,
                            "MON/DIRECT=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_flat_files_are_parsed_correctly(self):
        valid_settings = {"MON/FLat  = C:\path1\Path2\file.ext ": {user_file_flat_lab: "C:/path1/Path2/file.ext"},
                          "MON/ flAt  = filE.Ext ": {user_file_flat_lab: "filE.Ext"},
                          "MON/flAT= \path1\Path2\file.ext ": {user_file_flat_lab: "/path1/Path2/file.ext"},
                          "MON/FLat= /path1/Path2/file.ext ": {user_file_flat_lab: "/path1/Path2/file.ext"},
                          "MON/FLat= /path1/Path2/file.ext ": {user_file_flat_lab: "/path1/Path2/file.ext"},
                          "MON/FLat/ rear= /path1/Path2/file.ext ": {user_file_flat_lab: "/path1/Path2/file.ext"},
                          "MON/FLat/ frONT= path1/Path2/file.ext ": {user_file_flat_hab: "path1/Path2/file.ext"}}

        invalid_settings = {"MON/DIRECT= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/DIRECT /path1/Path2/file.ext ": RuntimeError,
                            "MON/DIRECT=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_hab_files_are_parsed_correctly(self):
        valid_settings = {"MON/HAB  = C:\path1\Path2\file.ext ": {user_file_mon_hab: "C:/path1/Path2/file.ext"},
                          "MON/ hAB  = filE.Ext ": {user_file_mon_hab: "filE.Ext"},
                          "MON/HAb= \path1\Path2\file.ext ": {user_file_mon_hab: "/path1/Path2/file.ext"},
                          "MON/hAB= /path1/Path2/file.ext ": {user_file_mon_hab: "/path1/Path2/file.ext"}}
        invalid_settings = {"MON/HAB= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/hAB /path1/Path2/file.ext ": RuntimeError,
                            "MON/HAB=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_hab_files_are_parsed_correctly(self):
        valid_settings = {"MON/Spectrum = 123 ": {user_file_mon_spectrum: 123},
                          "MON/trans/Spectrum = 123 ": {user_file_mon_spectrum_trans: 123},
                          "MON/trans/Spectrum = 123 /  interpolate": {user_file_mon_spectrum_trans: 123,
                                                        user_file_mon_interpolate: True},
                          "MON/Spectrum = 123 /  interpolate": {user_file_mon_spectrum: 123,
                                                                user_file_mon_interpolate: True}}
        invalid_settings = {}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class PrintParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(PrintParser.get_type(), "PRINT")

    def test_that_print_is_parsed_correctly(self):
        valid_settings = {"PRINT OdlfP slsk 23lksdl2 34l": {user_file_print: "OdlfP slsk 23lksdl2 34l"}}

        invalid_settings = {}

        print_parser = PrintParser()
        do_test(print_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class BackParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(BackParser.get_type(), "BACK")

    def test_that_all_monitors_is_parsed_correctly(self):
        valid_settings = {"BACK / MON /times  123 34": {user_file_all_monitors: [123, 34]}}

        invalid_settings = {}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_monitors_is_parsed_correctly(self):
        valid_settings = {"BACK / M3 /times  123 34": {user_file_single_monitors: [3, 123, 34]},
                          "BACK / M3 123 34": {user_file_single_monitors: [3, 123, 34]}}

        invalid_settings = {"BACK / M 123 34": RuntimeError,
                            "BACK / M3 123": RuntimeError}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_off_is_parsed_correctly(self):
        valid_settings = {"BACK / M3 /OFF": {user_file_monitor_off: 3}}

        invalid_settings = {"BACK / M /OFF": RuntimeError}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SANS2DParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SANS2DParser.get_type(), "SANS2D")

    def test_that_sans2d_is_parsed_correctly(self):
        sans2D_parser = SANS2DParser()
        result = sans2D_parser.parse_line("SANS2D ")
        self.assertTrue(result is None)


class LOQParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(LOQParser.get_type(), "LOQ")

    def test_that_loq_is_parsed_correctly(self):
        loq_parser = LOQParser()
        result = loq_parser.parse_line("LOQ ")
        self.assertTrue(result is None)


class UserFileParserTest(unittest.TestCase):
    def test_that_correct_parser_is_selected_(self):
        # Arrange
        user_file_parser = UserFileParser()

        # DetParser
        result = user_file_parser.parse_line(" DET/CoRR/FRONT/ SidE -957")
        assert_valid_result(result, {user_file_correction_translation_hab: -957}, self.assertTrue)

        # LimitParser
        result = user_file_parser.parse_line("l/Q/WCUT 234.4")
        assert_valid_result(result, {user_file_wavelength_cut: 234.4}, self.assertTrue)

        # MaskParser
        result = user_file_parser.parse_line("MASK S 12  ")
        assert_valid_result(result, {use_file_single_spectrum_mask: 12}, self.assertTrue)

        # SampleParser
        result = user_file_parser.parse_line("SAMPLE /Offset 234.5")
        assert_valid_result(result, {user_file_sample_offset: 234.5}, self.assertTrue)

        # TransParser
        result = user_file_parser.parse_line("TRANS / radius  =23")
        assert_valid_result(result, {user_file_trans_radius: 23}, self.assertTrue)

        # TubeCalibFileParser
        result = user_file_parser.parse_line("TUBECALIbfile= calib_file.nxs")
        assert_valid_result(result, {user_file_tube_calibration: "calib_file.nxs"}, self.assertTrue)

        # QResolutionParser
        result = user_file_parser.parse_line("QRESOLUTION/ON")
        assert_valid_result(result, {user_file_q_resolution_on: True}, self.assertTrue)

        # FitParser
        result = user_file_parser.parse_line("FIT/TRANS/Straight 123 3556")
        assert_valid_result(result, {user_file_range_based_fit_lin: [123, 3556]}, self.assertTrue)

        # GravityParser
        result = user_file_parser.parse_line("Gravity/LExtra =23.5")
        assert_valid_result(result, {user_file_gravity_extra_length: 23.5}, self.assertTrue)

        # MaskFileParser
        result = user_file_parser.parse_line("MaskFile= test.xml,   testKsdk2.xml,tesetlskd.xml")
        assert_valid_result(result, {user_file_mask_file: ["test.xml", "testKsdk2.xml", "tesetlskd.xml"]},
                            self.assertTrue)

        # MonParser
        result = user_file_parser.parse_line("MON/length= 23.5 34")
        assert_valid_result(result, {user_file_monitor_length: [23.5, 34]}, self.assertTrue)

        # PrintParser
        result = user_file_parser.parse_line("PRINT OdlfP slsk 23lksdl2 34l")
        assert_valid_result(result, {user_file_print: "OdlfP slsk 23lksdl2 34l"}, self.assertTrue)

        # BackParser
        result = user_file_parser.parse_line("BACK / M3 /OFF")
        assert_valid_result(result, {user_file_monitor_off: 3}, self.assertTrue)

        # SANS2DParser
        result = user_file_parser.parse_line("SANS2D")
        self.assertTrue(result is None)

        # LOQParser
        result = user_file_parser.parse_line("LOQ")
        self.assertTrue(result is None)

    def test_that_non_existent_parser_throws(self):
        # Arrange
        user_file_parser = UserFileParser()

        # Act + Assert
        self.assertRaises(ValueError, user_file_parser.parse_line, "DetT/DKDK/ 23 23")

if __name__ == "__main__":
    unittest.main()
