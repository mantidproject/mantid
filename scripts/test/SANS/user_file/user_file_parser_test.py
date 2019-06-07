# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.common.enums import (ISISReductionMode, DetectorType, RangeStepType, FitType, DataType, SANSInstrument)
from sans.user_file.user_file_parser import (InstrParser, DetParser, LimitParser, MaskParser, SampleParser, SetParser, TransParser,
                                             TubeCalibFileParser, QResolutionParser, FitParser, GravityParser,
                                             MaskFileParser, MonParser, PrintParser, BackParser, SANS2DParser, LOQParser,
                                             UserFileParser, LARMORParser, CompatibilityParser)
from sans.user_file.settings_tags import (DetectorId, BackId, range_entry, back_single_monitor_entry,
                                          single_entry_with_detector, mask_angle_entry, LimitsId,
                                          simple_range, complex_range, MaskId, mask_block, mask_block_cross,
                                          mask_line, range_entry_with_detector, SampleId, SetId, set_scales_entry,
                                          position_entry, TransId, TubeCalibrationFileId, QResolutionId, FitId,
                                          fit_general, MonId, monitor_length, monitor_file, GravityId, OtherId,
                                          monitor_spectrum, PrintId, det_fit_range, q_rebin_values)


# -----------------------------------------------------------------
# --- Free Helper Functions for Testing ---------------------------
# -----------------------------------------------------------------
def assert_valid_result(result, expected, assert_true):
    keys_result = list(result.keys())
    keys_expected = list(expected.keys())
    assert_true(len(keys_expected) == len(keys_result))
    for key in keys_result:
        assert_true(key in keys_expected)
        if result[key] != expected[key]:
            assert_true(result[key] == expected[key], "For key {}, {} does not equal {}".format(key, result[key],
                                                                                                expected[key]))


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


# # -----------------------------------------------------------------
# # --- Tests -------------------------------------------------------
# # -----------------------------------------------------------------
class InstrParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(InstrParser.get_type(), "INSTR")

    def test_that_instruments_are_recognised(self):
        self.assertTrue(InstrParser.get_type_pattern("LOQ"))
        self.assertTrue(InstrParser.get_type_pattern("SANS2D"))
        self.assertTrue(InstrParser.get_type_pattern("ZOOM"))
        self.assertTrue(InstrParser.get_type_pattern("LARMOR"))
        self.assertFalse(InstrParser.get_type_pattern("Not an instrument"))
        self.assertFalse(InstrParser.get_type_pattern("SANS2D/something else"))

    def test_that_instruments_are_parsed_correctly(self):
        valid_settings = {"SANS2D": {DetectorId.instrument: SANSInstrument.SANS2D},
                          "LOQ": {DetectorId.instrument: SANSInstrument.LOQ},
                          "ZOOM": {DetectorId.instrument: SANSInstrument.ZOOM},
                          "LARMOR": {DetectorId.instrument: SANSInstrument.LARMOR}}

        invalid_settings = {"NOINSTRUMENT": RuntimeError,
                            "SANS2D/HAB": RuntimeError,
                            "!LOQ": RuntimeError}

        instr_parser = InstrParser()
        do_test(instr_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class DetParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(DetParser.get_type(), "DET")

    def test_that_reduction_mode_is_parsed_correctly(self):
        # The dict below has the string to parse as the key and the expected result as a value
        valid_settings = {"DET/HAB": {DetectorId.reduction_mode: ISISReductionMode.HAB},
                          "dEt/ frONT ": {DetectorId.reduction_mode: ISISReductionMode.HAB},
                          "dET/REAR": {DetectorId.reduction_mode: ISISReductionMode.LAB},
                          "dEt/MAIn   ": {DetectorId.reduction_mode: ISISReductionMode.LAB},
                          " dEt/ BOtH": {DetectorId.reduction_mode: ISISReductionMode.All},
                          "DeT /merge ": {DetectorId.reduction_mode: ISISReductionMode.Merged},
                          " DEt / MERGED": {DetectorId.reduction_mode: ISISReductionMode.Merged}}

        invalid_settings = {"DET/HUB": RuntimeError,
                            "DET/HAB/": RuntimeError}
        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_merge_option_is_parsed_correctly(self):
        valid_settings = {"DET/RESCALE 123": {DetectorId.rescale: 123},
                          "dEt/ shiFt 48.5": {DetectorId.shift: 48.5},
                          "dET/reSCale/FIT   23 34.6 ": {DetectorId.rescale_fit: det_fit_range(start=23, stop=34.6,
                                                                                               use_fit=True)},
                          "dEt/SHIFT/FIT 235.2  341   ": {DetectorId.shift_fit: det_fit_range(start=235.2, stop=341,
                                                                                              use_fit=True)}}

        invalid_settings = {"DET/Ruscale": RuntimeError,
                            "DET/SHIFT/": RuntimeError,
                            "DET/SHIFT 1 2": RuntimeError,
                            "DET/SHIFT/FIT 1 ": RuntimeError,
                            "DET/Rescale/FIT 1 2 4": RuntimeError}

        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_detector_setting_is_parsed_correctly(self):
        valid_settings = {"Det/CORR/REAR/X 123": {DetectorId.correction_x: single_entry_with_detector(entry=123,
                                                                    detector_type=DetectorType.LAB)},  # noqa
                          "DEt/CORR/ frOnt/X +95.7": {DetectorId.correction_x:
                                                          single_entry_with_detector(entry=95.7,
                                                                                     detector_type=DetectorType.HAB)},
                          "DeT/ CORR / ReAR/ y 12.3": {DetectorId.correction_y:
                                                           single_entry_with_detector(entry=12.3,
                                                                                      detector_type=DetectorType.LAB)},
                          " DET/CoRR/fROnt/Y -957": {DetectorId.correction_y:
                                                         single_entry_with_detector(entry=-957,
                                                                                    detector_type=DetectorType.HAB)},
                          "DeT/ CORR /reAR/Z 12.3": {DetectorId.correction_z:
                                                         single_entry_with_detector(entry=12.3,
                                                                                    detector_type=DetectorType.LAB)},
                          " DET/CoRR/FRONT/ Z -957": {DetectorId.correction_z:
                                                          single_entry_with_detector(entry=-957,
                                                                                     detector_type=DetectorType.HAB)},
                          "DeT/ CORR /reAR/SIDE 12.3": {DetectorId.correction_translation:
                                                            single_entry_with_detector(entry=12.3,
                                                                                       detector_type=DetectorType.LAB)},
                          " DET/CoRR/FRONT/ SidE -957": {DetectorId.correction_translation:
                                                             single_entry_with_detector(entry=-957,
                                                                                    detector_type=DetectorType.HAB)},
                          "DeT/ CORR /reAR/ROt 12.3": {DetectorId.correction_rotation:
                                                           single_entry_with_detector(entry=12.3,
                                                                                      detector_type=DetectorType.LAB)},
                          " DET/CoRR/FRONT/ROT -957": {DetectorId.correction_rotation:
                                                           single_entry_with_detector(entry=-957,
                                                                                      detector_type=DetectorType.HAB)},
                          "DeT/ CORR /reAR/Radius 12.3": {DetectorId.correction_radius:
                                                              single_entry_with_detector(entry=12.3,
                                                                                     detector_type=DetectorType.LAB)},
                          " DET/CoRR/FRONT/RADIUS 957": {DetectorId.correction_radius:
                                                             single_entry_with_detector(entry=957,
                                                                                     detector_type=DetectorType.HAB)}}

        invalid_settings = {"Det/CORR/REAR/X ": RuntimeError,
                            "DEt/CORR/ frOnt/X 12 23": RuntimeError,
                            " DET/CoRR/fROnt": RuntimeError,
                            "DeT/ CORR /reAR/Z test": RuntimeError,
                            " DET/CoRR/FRONT/ ZZ -957": RuntimeError,
                            "DeT/ CORR /reAR/SIDE D 12.3": RuntimeError,
                            " DET/CoRR/FRONT/ SidE -i3": RuntimeError}

        det_parser = DetParser()

        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_DET_OVERLAP_option_is_parsed_correctly(self):
        valid_settings = {"DET/OVERLAP 0.13 0.15": {DetectorId.merge_range: det_fit_range(start=0.13, stop=0.15, use_fit=True)},
                          "DeT/OverLAP 0.13 0.15": {DetectorId.merge_range: det_fit_range(start=0.13, stop=0.15, use_fit=True)}
                          }

        invalid_settings = {"DET/OVERLAP 0.13 0.15 0.17": RuntimeError,
                            "DET/OVERLAP 0.13": RuntimeError,
                            "DET/OVERLAP": RuntimeError,
                            "DET/OVERLAP 0.13 five": RuntimeError}

        det_parser = DetParser()
        do_test(det_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class LimitParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(LimitParser.get_type(), "L")

    def test_that_angle_limit_is_parsed_correctly(self):
        valid_settings = {"L/PhI 123   345.2": {LimitsId.angle: mask_angle_entry(min=123, max=345.2,
                                                                                 use_mirror=True)},
                          "L/PHI / NOMIRROR 123 -345.2": {LimitsId.angle: mask_angle_entry(min=123, max=-345.2,
                                                                                           use_mirror=False)}}

        invalid_settings = {"L/PHI/NMIRROR/ 23 454": RuntimeError,
                            "L /pHI/ 23": RuntimeError,
                            "L/PhI/ f f": RuntimeError,
                            "L/ PHI/ f f": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_event_time_limit_is_parsed_correctly(self):
        valid_settings = {"L  / EVEnTStime 0,-10,32,434,34523,35": {LimitsId.events_binning:
                                                                    "0.0,-10.0,32.0,434.0,34523.0,35.0"},
                          "L / Eventstime 0 -10 32 434 34523 35": {LimitsId.events_binning:
                                                                   "0.0,-10.0,32.0,434.0,34523.0,35.0"}}

        invalid_settings = {"L  / EEnTStime 0,-10,32,434,34523,35": RuntimeError,
                            "L/EVENTSTIME 123g, sdf": RuntimeError,
                            "L  /EvEnTStime": RuntimeError}
        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_cut_limits_are_parsed_correctly(self):
        valid_settings = {"L/Q/RCUT 234.4": {LimitsId.radius_cut: 234.4},
                          "L /q / RcUT -234.34": {LimitsId.radius_cut: -234.34},
                          "l/Q/WCUT 234.4": {LimitsId.wavelength_cut: 234.4},
                          "L /q / wcUT -234.34": {LimitsId.wavelength_cut: -234.34}}

        invalid_settings = {"L/Q/Rcu 123": RuntimeError,
                            "L/Q/RCUT/ 2134": RuntimeError,
                            "L/Q/Wcut 23 234": RuntimeError,
                            "L/Q/WCUT": RuntimeError,
                            "L / Q / WCUT234": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_radius_limits_are_parsed_correctly(self):
        valid_settings = {"L/R 234 235": {LimitsId.radius: range_entry(start=234, stop=235)},
                          "L / r   -234   235": {LimitsId.radius: range_entry(start=-234, stop=235)},
                          "L / r   -234   235 454": {LimitsId.radius: range_entry(start=-234, stop=235)}
                          }
        invalid_settings = {"L/R/ 234 435": RuntimeError,
                            "L/Rr 234 435": RuntimeError,
                            "L/R 435": RuntimeError,
                            "L/R sdf": RuntimeError,
                            "L/R": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_q_limits_are_parsed_correctly(self):
        valid_settings = {"L/Q 12 34": {LimitsId.q: q_rebin_values(min=12., max=34., rebin_string="12.0,34.0")},
                          "L/Q 12 34 2.7": {LimitsId.q: q_rebin_values(min=12., max=34., rebin_string="12.0,2.7,34.0")},
                          "L/Q -12 34.6 2.7/LOG": {LimitsId.q: q_rebin_values(min=-12., max=34.6,
                                                                              rebin_string="-12.0,-2.7,34.6")},
                          "L/q -12 3.6 2 /LIN": {LimitsId.q: q_rebin_values(min=-12., max=3.6,
                                                                            rebin_string="-12.0,2.0,3.6")},
                          "L/q -12   0.41  23 -34.8 3.6": {LimitsId.q: q_rebin_values(min=-12., max=3.6,
                                                              rebin_string="-12.0,0.41,23.0,-34.8,3.6")},  # noqa
                          "L/q -12   0.42  23 -34.8 3.6 /LIn": {LimitsId.q: q_rebin_values(min=-12., max=3.6,
                                                                   rebin_string="-12.0,0.42,23.0,34.8,3.6")},
                          "L/q -12   0.43     23 -34.8 3.6": {LimitsId.q: q_rebin_values(min=-12., max=3.6,
                                                                            rebin_string="-12.0,0.43,23.0,-34.8,3.6")},
                          "L/q -12   0.44  23  ,34.8,3.6  /Log": {LimitsId.q: q_rebin_values(min=-12., max=3.6,
                                                                    rebin_string="-12.0,-0.44,23.0,-34.8,3.6")},
                          "L/q -12  , 0.45 , 23  ,34.8 ,3.6, .123, 5.6  /Log": {LimitsId.q: q_rebin_values(min=-12.,
                                                                                                          max=5.6,
                                                                    rebin_string="-12.0,-0.45,23.0,-34.8,3.6,"
                                                                                 "-0.123,5.6")},
                          "L/q -12  , 0.46 , 23  ,34.8 ,3.6, -.123, 5.6": {LimitsId.q: q_rebin_values(min=-12.,
                                                                                                     max=5.6,
                                                                          rebin_string="-12.0,0.46,23.0,34.8,3.6,"
                                                                                       "-0.123,5.6")},
                          "L/q -12   0.47   23 34.8  3.6, -.123    5.6": {LimitsId.q: q_rebin_values(min=-12.,
                                                                            max=5.6,
                                                                            rebin_string="-12.0,0.47,23.0,34.8,3.6,"
                                                                                            "-0.123,5.6")}
                          }

        invalid_settings = {"L/Q 12 2 3 4 5/LUG": RuntimeError,
                            "L/Q 12 2 /LIN": RuntimeError,
                            "L/Q ": RuntimeError,
                            "L/Q a 1 2 3 4 /LIN": RuntimeError}
        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_qxy_limits_are_parsed_correctly(self):
        valid_settings = {"L/QXY 12 34": {LimitsId.qxy: simple_range(start=12, stop=34, step=None,  step_type=None)},
                          "L/QXY 12 34 2.7": {LimitsId.qxy: simple_range(start=12, stop=34, step=2.7,
                                                                         step_type=RangeStepType.Lin)},
                          "L/QXY -12 34.6 2.7/LOG": {LimitsId.qxy: simple_range(start=-12, stop=34.6, step=2.7,
                                                                                step_type=RangeStepType.Log)},
                          "L/qxY -12 3.6 2 /LIN": {LimitsId.qxy: simple_range(start=-12, stop=3.6, step=2,
                                                                              step_type=RangeStepType.Lin)}}
        """
        These tests should be added back to valid settings when SANS GUI can accept complex QXY strings.
        "L/qxy -12  , 0.4,  23, -3.48, 36": {LimitsId.qxy: complex_range(start=-12, step1=0.4,
                                               mid=23, step2=3.48, stop=36,
                                               step_type1=RangeStepType.Lin,
                                               step_type2=RangeStepType.Log)},
        "L/qXY -12   0.4  23 3.48 36 /LIn": {LimitsId.qxy: complex_range(start=-12,
                                                    step1=0.4, mid=23, step2=3.48, stop=36,
                                                    step_type1=RangeStepType.Lin,
                                                    step_type2=RangeStepType.Lin)},
        "L/qXY -12   0.4  23  3.48 36  /Log": {LimitsId.qxy: complex_range(start=-12,
                                                      step1=0.4, mid=23, step2=3.48, stop=36,
                                                      step_type1=RangeStepType.Log,
                                                      step_type2=RangeStepType.Log)}"""

        invalid_settings = {"L/QXY 12 2 3 4": RuntimeError,
                            "L/QXY 12 2 3 4 23 3": RuntimeError,
                            "L/QXY 12 2 3 4 5/LUG": RuntimeError,
                            "L/QXY 12 2 /LIN": RuntimeError,
                            "L/QXY ": RuntimeError,
                            "L/QXY a 1 2 3 4 /LIN": RuntimeError}

        limit_parser = LimitParser()
        do_test(limit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_wavelength_limits_are_parsed_correctly(self):
        valid_settings = {"L/WAV 12 34": {LimitsId.wavelength: simple_range(start=12, stop=34, step=None,
                                                                            step_type=None)},
                          "L/waV 12 34 2.7": {LimitsId.wavelength: simple_range(start=12, stop=34, step=2.7,
                                                                                step_type=RangeStepType.Lin)},
                          "L/wAv -12 34.6 2.7/LOG": {LimitsId.wavelength: simple_range(start=-12, stop=34.6, step=2.7,
                                                                                       step_type=RangeStepType.Log)},
                          "L/WaV -12 3.6 2 /LIN": {LimitsId.wavelength: simple_range(start=-12, stop=3.6,  step=2,
                                                                                     step_type=RangeStepType.Lin)}}

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
        valid_settings = {"MASK/LiNE 12  23.6": {MaskId.line: mask_line(width=12, angle=23.6, x=None, y=None)},
                          "MASK/LiNE 12  23.6 2 346": {MaskId.line: mask_line(width=12, angle=23.6, x=2, y=346)}
                          }
        invalid_settings = {"MASK/LiN 12 4": RuntimeError,
                            "MASK/LINE 12": RuntimeError,
                            "MASK/LINE 12 34 345 6 7": RuntimeError,
                            "MASK/LINE ": RuntimeError,
                            "MASK/LINE  x y": RuntimeError,
                            }

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_masked_time_is_parsed_correctly(self):
        valid_settings = {"MASK/TIME 23 35": {MaskId.time: range_entry_with_detector(start=23, stop=35,
                                                                                     detector_type=None)},
                          "MASK/T 23 35": {MaskId.time: range_entry_with_detector(start=23, stop=35,
                                                                                  detector_type=None)},
                          "MASK/REAR/T 13 35": {MaskId.time_detector: range_entry_with_detector(start=13, stop=35,
                                                detector_type=DetectorType.LAB)},
                          "MASK/FRONT/TIME 33 35": {MaskId.time_detector: range_entry_with_detector(start=33, stop=35,
                                                    detector_type=DetectorType.HAB)},
                          "MASK/TIME/REAR 13 35": {MaskId.time_detector: range_entry_with_detector(start=13, stop=35,
                                                   detector_type=DetectorType.LAB)},
                          "MASK/T/FRONT 33 35": {MaskId.time_detector: range_entry_with_detector(start=33, stop=35,
                                                 detector_type=DetectorType.HAB)}
                          }

        invalid_settings = {"MASK/TIME 12 34 4 ": RuntimeError,
                            "MASK/T 2": RuntimeError,
                            "MASK/T": RuntimeError,
                            "MASK/T x y": RuntimeError,
                            "MASK/REA/T 12 13": RuntimeError}

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_clear_mask_is_parsed_correctly(self):
        valid_settings = {"MASK/CLEAR": {MaskId.clear_detector_mask: True},
                          "MASK/CLeaR /TIMe": {MaskId.clear_time_mask: True}}

        invalid_settings = {"MASK/CLEAR/TIME/test": RuntimeError,
                            "MASK/CLEAR/TIIE": RuntimeError,
                            "MASK/CLEAR test": RuntimeError}

        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_spectrum_mask_is_parsed_correctly(self):
        valid_settings = {"MASK S 12  ": {MaskId.single_spectrum_mask: 12},
                          "MASK S234": {MaskId.single_spectrum_mask: 234}}

        invalid_settings = {"MASK B 12  ": RuntimeError,
                            "MASK S 12 23 ": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_spectrum_range_is_parsed_correctly(self):
        valid_settings = {"MASK S 12 >  S23  ": {MaskId.spectrum_range_mask: range_entry(start=12, stop=23)},
                          "MASK S234>S1234": {MaskId.spectrum_range_mask: range_entry(start=234, stop=1234)}}

        invalid_settings = {"MASK S 12> S123.5  ": RuntimeError,
                            "MASK S 12> 23 ": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_vertical_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK V 12  ": {MaskId.vertical_single_strip_mask: single_entry_with_detector(entry=12,
                                          detector_type=DetectorType.LAB)},
                          "MASK / Rear V  12  ": {MaskId.vertical_single_strip_mask: single_entry_with_detector(
                                                  entry=12, detector_type=DetectorType.LAB)},
                          "MASK/mAin V234": {MaskId.vertical_single_strip_mask: single_entry_with_detector(entry=234,
                                             detector_type=DetectorType.LAB)},
                          "MASK / LaB V  234": {MaskId.vertical_single_strip_mask: single_entry_with_detector(entry=234,
                                                detector_type=DetectorType.LAB)},
                          "MASK /frOnt V  12  ": {MaskId.vertical_single_strip_mask: single_entry_with_detector(
                                                  entry=12, detector_type=DetectorType.HAB)},
                          "MASK/HAB V234": {MaskId.vertical_single_strip_mask:  single_entry_with_detector(entry=234,
                                            detector_type=DetectorType.HAB)}}

        invalid_settings = {"MASK B 12  ": RuntimeError,
                            "MASK V 12 23 ": RuntimeError,
                            "MASK \Rear V3": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_range_vertical_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK V  12 >  V23  ": {MaskId.vertical_range_strip_mask: range_entry_with_detector(start=12,
                                                  stop=23, detector_type=DetectorType.LAB)},
                          "MASK V123>V234": {MaskId.vertical_range_strip_mask: range_entry_with_detector(start=123,
                                             stop=234, detector_type=DetectorType.LAB)},
                          "MASK / Rear V123>V234": {MaskId.vertical_range_strip_mask:  range_entry_with_detector(
                                                    start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK/mAin  V123>V234": {MaskId.vertical_range_strip_mask: range_entry_with_detector(
                                                   start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK / LaB V123>V234": {MaskId.vertical_range_strip_mask: range_entry_with_detector(
                                                   start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK/frOnt V123>V234": {MaskId.vertical_range_strip_mask: range_entry_with_detector(
                                                   start=123, stop=234, detector_type=DetectorType.HAB)},
                          "MASK/HAB V123>V234": {MaskId.vertical_range_strip_mask: range_entry_with_detector(
                                                 start=123, stop=234, detector_type=DetectorType.HAB)}}

        invalid_settings = {"MASK V 12> V123.5  ": RuntimeError,
                            "MASK V 12 23 ": RuntimeError,
                            "MASK /Rear/ V12>V34": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_horizontal_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK H 12  ": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(entry=12,
                                          detector_type=DetectorType.LAB)},
                          "MASK / Rear H  12  ": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(
                                                  entry=12, detector_type=DetectorType.LAB)},
                          "MASK/mAin H234": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(entry=234,
                                             detector_type=DetectorType.LAB)},
                          "MASK / LaB H  234": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(
                                                entry=234, detector_type=DetectorType.LAB)},
                          "MASK /frOnt H  12  ": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(
                                                  entry=12, detector_type=DetectorType.HAB)},
                          "MASK/HAB H234": {MaskId.horizontal_single_strip_mask: single_entry_with_detector(entry=234,
                                            detector_type=DetectorType.HAB)}}

        invalid_settings = {"MASK H/12  ": RuntimeError,
                            "MASK H 12 23 ": RuntimeError,
                            "MASK \Rear H3": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_range_horizontal_strip_mask_is_parsed_correctly(self):
        valid_settings = {"MASK H  12 >  H23  ": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                                  start=12, stop=23, detector_type=DetectorType.LAB)},
                          "MASK H123>H234": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                             start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK / Rear H123>H234": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                                    start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK/mAin H123>H234": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                                  start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK / LaB H123>H234": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                                   start=123, stop=234, detector_type=DetectorType.LAB)},
                          "MASK/frOnt H123>H234": {MaskId.horizontal_range_strip_mask: range_entry_with_detector(
                                                   start=123, stop=234, detector_type=DetectorType.HAB)},
                          "MASK/HAB H123>H234": {MaskId.horizontal_range_strip_mask:  range_entry_with_detector(
                                                 start=123, stop=234, detector_type=DetectorType.HAB)}}

        invalid_settings = {"MASK H 12> H123.5  ": RuntimeError,
                            "MASK H 12 23 ": RuntimeError,
                            "MASK /Rear/ H12>V34": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_block_mask_is_parsed_correctly(self):
        valid_settings = {"MASK H12>H23 + V14>V15 ": {MaskId.block: mask_block(horizontal1=12, horizontal2=23,
                                                                               vertical1=14, vertical2=15,
                                                                               detector_type=DetectorType.LAB)},
                          "MASK/ HAB H12>H23 + V14>V15 ": {MaskId.block: mask_block(horizontal1=12, horizontal2=23,
                                                                                    vertical1=14, vertical2=15,
                                                                                    detector_type=DetectorType.HAB)},
                          "MASK/ HAB V12>V23 + H14>H15 ": {MaskId.block: mask_block(horizontal1=14, horizontal2=15,
                                                                                    vertical1=12, vertical2=23,
                                                                                    detector_type=DetectorType.HAB)},
                          "MASK  V12 + H 14": {MaskId.block_cross: mask_block_cross(horizontal=14, vertical=12,
                                                                                    detector_type=DetectorType.LAB)},
                          "MASK/HAB H12 + V 14": {MaskId.block_cross: mask_block_cross(horizontal=12, vertical=14,
                                                                                       detector_type=DetectorType.HAB)}}

        invalid_settings = {"MASK H12>H23 + V14 + V15 ": RuntimeError,
                            "MASK H12 + H15 ": RuntimeError,
                            "MASK/ HAB V12 + H14>H15 ": RuntimeError}
        mask_parser = MaskParser()
        do_test(mask_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SampleParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SampleParser.get_type(), "SAMPLE")

    def test_that_setting_sample_path_is_parsed_correctly(self):
        valid_settings = {"SAMPLE /PATH/ON": {SampleId.path: True},
                          "SAMPLE / PATH / OfF": {SampleId.path: False}}

        invalid_settings = {"SAMPLE/PATH ON": RuntimeError,
                            "SAMPLE /pATh ": RuntimeError,
                            "SAMPLE/ Path ONN": RuntimeError}

        sample_parser = SampleParser()
        do_test(sample_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_setting_sample_offset_is_parsed_correctly(self):
        valid_settings = {"SAMPLE /Offset 234.5": {SampleId.offset: 234.5},
                          "SAMPLE / Offset 25": {SampleId.offset: 25}}

        invalid_settings = {"SAMPL/offset fg": RuntimeError,
                            "SAMPLE /Offset/ 23 ": RuntimeError,
                            "SAMPLE/ offset 234 34": RuntimeError}

        sample_parser = SampleParser()
        do_test(sample_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SetParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SetParser.get_type(), "SET")

    def test_that_setting_scales_is_parsed_correctly(self):
        valid_settings = {"SET  scales 2 5 4    7 8": {SetId.scales: set_scales_entry(s=2, a=5, b=4, c=7, d=8)}}

        invalid_settings = {"SET scales 2 4 6 7 8 9": RuntimeError,
                            "SET scales ": RuntimeError}

        set_parser = SetParser()
        do_test(set_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_centre_is_parsed_correctly(self):
        valid_settings = {"SET centre 23 45": {SetId.centre: position_entry(pos1=23, pos2=45,
                                                                            detector_type=DetectorType.LAB)},
                          "SET centre /main 23 45": {SetId.centre: position_entry(pos1=23, pos2=45,
                                                                                  detector_type=DetectorType.LAB)},
                          "SET centre / lAb 23 45": {SetId.centre: position_entry(pos1=23, pos2=45,
                                                                                  detector_type=DetectorType.LAB)},
                          "SET centre / hAb 23 45": {SetId.centre_HAB: position_entry(pos1=23, pos2=45,
                                                                                  detector_type=DetectorType.HAB)},
                          "SET centre /FRONT 23 45": {SetId.centre_HAB: position_entry(pos1=23, pos2=45,
                                                      detector_type=DetectorType.HAB)},
                          "SET centre /FRONT 23 45 55 67": {SetId.centre_HAB: position_entry(pos1=23, pos2=45,
                                                            detector_type=DetectorType.HAB)},
                          }

        invalid_settings = {"SET centre 23": RuntimeError,
                            "SEt centre 34 34 23 ": RuntimeError,
                            "SEt centre/MAIN/ 34 34": RuntimeError,
                            "SEt centre/MAIN": RuntimeError}

        set_parser = SetParser()
        do_test(set_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class TransParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(TransParser.get_type(), "TRANS")

    def test_that_trans_spec_is_parsed_correctly(self):
        valid_settings = {"TRANS/TRANSPEC=23": {TransId.spec: 23},
                          "TRANS / TransPEC =  23": {TransId.spec: 23}}

        invalid_settings = {"TRANS/TRANSPEC 23": RuntimeError,
                            "TRANS/TRANSPEC/23": RuntimeError,
                            "TRANS/TRANSPEC=23.5": RuntimeError,
                            "TRANS/TRANSPEC=2t": RuntimeError,
                            "TRANS/TRANSSPEC=23": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_trans_spec_shift_is_parsed_correctly(self):
        valid_settings = {"TRANS/TRANSPEC=4/SHIFT=23": {TransId.spec_shift: 23, TransId.spec: 4},
                          "TRANS/TRANSPEC =4/ SHIFT = 23": {TransId.spec_shift: 23, TransId.spec: 4},
                          "TRANS/TRANSPEC =6/ SHIFT = 23": {TransId.spec_shift: 23, TransId.spec: 6},
                          }

        invalid_settings = {"TRANS/TRANSPEC=4/SHIFT/23": RuntimeError,
                            "TRANS/TRANSPEC=4/SHIFT 23": RuntimeError,
                            "TRANS/TRANSPEC/SHIFT=23": RuntimeError,
                            "TRANS/TRANSPEC=6/SHIFT=t": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_radius_is_parsed_correctly(self):
        valid_settings = {"TRANS / radius  =23": {TransId.radius: 23},
                          "TRANS /RADIUS= 245.7": {TransId.radius: 245.7}}
        invalid_settings = {}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_roi_is_parsed_correctly(self):
        valid_settings = {"TRANS/ROI =testFile.xml": {TransId.roi: ["testFile.xml"]},
                          "TRANS/ROI =testFile.xml, "
                          "TestFile2.XmL,testFile4.xml": {TransId.roi: ["testFile.xml", "TestFile2.XmL",
                                                                        "testFile4.xml"]}}
        invalid_settings = {"TRANS/ROI =t estFile.xml": RuntimeError,
                            "TRANS/ROI =testFile.txt": RuntimeError,
                            "TRANS/ROI testFile.txt": RuntimeError,
                            "TRANS/ROI=": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_mask_is_parsed_correctly(self):
        valid_settings = {"TRANS/Mask =testFile.xml": {TransId.mask: ["testFile.xml"]},
                          "TRANS/ MASK =testFile.xml, "
                          "TestFile2.XmL,testFile4.xml": {TransId.mask: ["testFile.xml", "TestFile2.XmL",
                                                                         "testFile4.xml"]}}
        invalid_settings = {"TRANS/MASK =t estFile.xml": RuntimeError,
                            "TRANS/  MASK =testFile.txt": RuntimeError,
                            "TRANS/ MASK testFile.txt": RuntimeError,
                            "TRANS/MASK=": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_workspaces_are_parsed_correctly(self):
        valid_settings = {"TRANS/SampleWS =testworksaoe234Name": {TransId.sample_workspace: "testworksaoe234Name"},
                          "TRANS/ SampleWS = testworksaoe234Name": {TransId.sample_workspace: "testworksaoe234Name"},
                          "TRANS/ CanWS =testworksaoe234Name": {TransId.can_workspace: "testworksaoe234Name"},
                          "TRANS/ CANWS = testworksaoe234Name": {TransId.can_workspace: "testworksaoe234Name"}}
        invalid_settings = {"TRANS/CANWS/ test": RuntimeError,
                            "TRANS/SAMPLEWS =": RuntimeError}

        trans_parser = TransParser()
        do_test(trans_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class TubeCalibFileParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(TubeCalibFileParser.get_type(), "TRANS")

    def test_that_tube_calibration_file_is_parsed_correctly(self):
        valid_settings = {"TUBECALIbfile= calib_file.nxs": {TubeCalibrationFileId.file: "calib_file.nxs"},
                          " tUBECALIBfile=  caAlib_file.Nxs": {TubeCalibrationFileId.file: "caAlib_file.Nxs"}}

        invalid_settings = {"TUBECALIFILE file.nxs": RuntimeError,
                            "TUBECALIBFILE=file.txt": RuntimeError,
                            "TUBECALIBFILE=file": RuntimeError}

        tube_calib_file_parser = TubeCalibFileParser()
        do_test(tube_calib_file_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class QResolutionParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(QResolutionParser.get_type(), "QRESOL")

    def test_that_q_resolution_on_off_is_parsed_correctly(self):
        valid_settings = {"QRESOL/ON": {QResolutionId.on: True},
                          "QREsoL / oFF": {QResolutionId.on: False}}

        invalid_settings = {"QRESOL= ON": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_q_resolution_float_values_are_parsed_correctly(self):
        valid_settings = {"QRESOL/deltaR = 23.546": {QResolutionId.delta_r: 23.546},
                          "QRESOL/ Lcollim = 23.546": {QResolutionId.collimation_length: 23.546},
                          "QRESOL/ a1 = 23.546": {QResolutionId.a1: 23.546},
                          "QRESOL/ a2 =  23": {QResolutionId.a2: 23},
                          "QRESOL /  H1 = 23.546 ": {QResolutionId.h1: 23.546},
                          "QRESOL /h2 = 23.546 ": {QResolutionId.h2: 23.546},
                          "QRESOL /  W1 = 23.546 ": {QResolutionId.w1: 23.546},
                          "QRESOL /W2 = 23.546 ": {QResolutionId.w2: 23.546}
                          }

        invalid_settings = {"QRESOL/DELTAR 23": RuntimeError,
                            "QRESOL /DELTAR = test": RuntimeError,
                            "QRESOL /A1 t": RuntimeError,
                            "QRESOL/B1=10": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_moderator_is_parsed_correctly(self):
        valid_settings = {"QRESOL/MODERATOR = test_file.txt": {QResolutionId.moderator: "test_file.txt"}}

        invalid_settings = {"QRESOL/MODERATOR = test_file.nxs": RuntimeError,
                            "QRESOL/MODERATOR/test_file.txt": RuntimeError,
                            "QRESOL/MODERATOR=test_filetxt": RuntimeError}

        q_resolution_parser = QResolutionParser()
        do_test(q_resolution_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class FitParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(FitParser.get_type(), "FIT")

    def test_that_general_fit_is_parsed_correctly(self):
        valid_settings = {"FIT/ trans / LIN 123 3556": {FitId.general: fit_general(start=123, stop=3556,
                                                        fit_type=FitType.Linear, data_type=None, polynomial_order=0)},
                          "FIT/ tranS/linear 123 3556": {FitId.general: fit_general(start=123, stop=3556,
                                                         fit_type=FitType.Linear, data_type=None, polynomial_order=0)},
                          "FIT/TRANS/Straight 123 3556": {FitId.general: fit_general(start=123, stop=3556,
                                                          fit_type=FitType.Linear, data_type=None, polynomial_order=0)},
                          "FIT/ tranS/LoG 123  3556.6 ": {FitId.general: fit_general(start=123, stop=3556.6,
                                                          fit_type=FitType.Logarithmic, data_type=None, polynomial_order=0)},  # noqa
                          "FIT/TRANS/  YlOG 123   3556": {FitId.general: fit_general(start=123, stop=3556,
                                                          fit_type=FitType.Logarithmic, data_type=None, polynomial_order=0)},  # noqa
                          "FIT/Trans/Lin": {FitId.general: fit_general(start=None, stop=None, fit_type=FitType.Linear,
                                                                       data_type=None, polynomial_order=0)},
                          "FIT/Trans/ Log": {FitId.general: fit_general(start=None, stop=None, fit_type=FitType.Logarithmic,  # noqa
                                                                        data_type=None, polynomial_order=0)},
                          "FIT/Trans/ polYnomial": {FitId.general: fit_general(start=None, stop=None,
                                                    fit_type=FitType.Polynomial, data_type=None, polynomial_order=2)},
                          "FIT/Trans/ polYnomial 3": {FitId.general: fit_general(start=None, stop=None,
                                                                                 fit_type=FitType.Polynomial,
                                                                                 data_type=None, polynomial_order=3)},
                          "FIT/Trans/Sample/Log 23.4 56.7": {FitId.general: fit_general(start=23.4, stop=56.7,
                                                             fit_type=FitType.Logarithmic, data_type=DataType.Sample,
                                                                                        polynomial_order=0)},
                          "FIT/Trans/can/ lIn 23.4 56.7": {FitId.general: fit_general(start=23.4, stop=56.7,
                                                           fit_type=FitType.Linear, data_type=DataType.Can,
                                                                                      polynomial_order=0)},
                          "FIT/Trans / can/polynomiAL 5 23 45": {FitId.general: fit_general(start=23, stop=45,
                                                                 fit_type=FitType.Polynomial, data_type=DataType.Can,
                                                                                            polynomial_order=5)},
                          "FIT/ trans / clear": {FitId.general: fit_general(start=None, stop=None,
                                                 fit_type=FitType.NoFit, data_type=None, polynomial_order=None)},
                          "FIT/traNS /ofF": {FitId.general: fit_general(start=None, stop=None,
                                             fit_type=FitType.NoFit, data_type=None, polynomial_order=None)}
                          }

        invalid_settings = {"FIT/TRANS/ YlOG 123": RuntimeError,
                            "FIT/TRANS/ YlOG 123 34 34": RuntimeError,
                            "FIT/TRANS/ YlOG 123 fg": RuntimeError,
                            "FIT/Trans / can/polynomiAL 6": RuntimeError,
                            "FIT/Trans /": RuntimeError,
                            "FIT/Trans / Lin 23": RuntimeError,
                            "FIT/Trans / lin 23 5 6": RuntimeError,
                            "FIT/Trans / lin 23 t": RuntimeError,
                            "FIT/  clear": RuntimeError,
                            "FIT/MONITOR/OFF": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_monitor_times_are_parsed_correctly(self):
        valid_settings = {"FIT/monitor 12 34.5": {FitId.monitor_times: range_entry(start=12, stop=34.5)},
                          "Fit / Monitor 12.6 34.5": {FitId.monitor_times: range_entry(start=12.6, stop=34.5)}}

        invalid_settings = {"Fit / Monitor 12.6 34 34": RuntimeError,
                            "Fit / Monitor": RuntimeError}

        fit_parser = FitParser()
        do_test(fit_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class GravityParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(GravityParser.get_type(), "GRAVITY")

    def test_that_gravity_on_off_is_parsed_correctly(self):
        valid_settings = {"Gravity on ": {GravityId.on_off: True},
                          "Gravity   OFF ": {GravityId.on_off: False}}

        invalid_settings = {"Gravity ": RuntimeError,
                            "Gravity ONN": RuntimeError}

        gravity_parser = GravityParser()
        do_test(gravity_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_gravity_extra_length_is_parsed_correctly(self):
        valid_settings = {"Gravity/LExtra =23.5": {GravityId.extra_length: 23.5},
                          "Gravity  / lExtra =  23.5": {GravityId.extra_length: 23.5},
                          "Gravity  / lExtra  23.5": {GravityId.extra_length: 23.5}}

        invalid_settings = {"Gravity/LExtra - 23.5": RuntimeError,
                            "Gravity/LExtra =tw": RuntimeError}

        gravity_parser = GravityParser()
        do_test(gravity_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class CompatibilityParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(CompatibilityParser.get_type(), "COMPATIBILITY")

    def test_that_compatibility_on_off_is_parsed_correctly(self):
        valid_settings = {"COMPATIBILITY on ": {OtherId.use_compatibility_mode: True},
                          "COMPATIBILITY   OFF ": {OtherId.use_compatibility_mode: False}}

        invalid_settings = {"COMPATIBILITY ": RuntimeError,
                            "COMPATIBILITY ONN": RuntimeError}

        compatibility_parser = CompatibilityParser()
        do_test(compatibility_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class MaskFileParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(MaskFileParser.get_type(), "MASKFILE")

    def test_that_gravity_on_off_is_parsed_correctly(self):
        valid_settings = {"MaskFile= test.xml,   testKsdk2.xml,tesetlskd.xml":
                          {MaskId.file: ["test.xml", "testKsdk2.xml", "tesetlskd.xml"]}}

        invalid_settings = {"MaskFile=": RuntimeError,
                            "MaskFile=test.txt": RuntimeError,
                            "MaskFile test.xml, test2.xml": RuntimeError}

        mask_file_parser = MaskFileParser()
        do_test(mask_file_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class MonParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(MonParser.get_type(), "MON")

    def test_that_length_is_parsed_correctly(self):
        valid_settings = {"MON/length= 23.5 34": {MonId.length: monitor_length(length=23.5, spectrum=34,
                                                                               interpolate=False)},
                          "MON/length= 23.5 34  / InterPolate": {MonId.length: monitor_length(length=23.5, spectrum=34,
                                                                                              interpolate=True)}}

        invalid_settings = {"MON/length= 23.5 34.7": RuntimeError,
                            "MON/length 23.5 34": RuntimeError,
                            "MON/length=23.5": RuntimeError,
                            "MON/length/23.5 34": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_direct_files_are_parsed_correctly(self):
        valid_settings = {"MON/DIRECT= C:\path1\Path2\file.ext ": {MonId.direct: [monitor_file(
                          file_path="C:/path1/Path2/file.ext", detector_type=DetectorType.HAB),
                          monitor_file(file_path="C:/path1/Path2/file.ext", detector_type=DetectorType.LAB)]},
                          "MON/ direct  = filE.Ext ": {MonId.direct: [monitor_file(file_path="filE.Ext",
                                                       detector_type=DetectorType.HAB), monitor_file(
                                                       file_path="filE.Ext", detector_type=DetectorType.LAB)
                                                       ]},
                          "MON/DIRECT= \path1\Path2\file.ext ": {MonId.direct: [monitor_file(
                                                                 file_path="/path1/Path2/file.ext",
                                                                 detector_type=DetectorType.HAB),
                                                                 monitor_file(file_path="/path1/Path2/file.ext",
                                                                              detector_type=DetectorType.LAB)]},
                          "MON/DIRECT= /path1/Path2/file.ext ": {MonId.direct: [monitor_file(
                                                                                file_path="/path1/Path2/file.ext",
                                                                                detector_type=DetectorType.HAB),
                                                                 monitor_file(file_path="/path1/Path2/file.ext",
                                                                              detector_type=DetectorType.LAB)]},
                          "MON/DIRECT/ rear= /path1/Path2/file.ext ": {MonId.direct: [monitor_file(
                                                                       file_path="/path1/Path2/file.ext",
                                                                       detector_type=DetectorType.LAB)]},
                          "MON/DIRECT/ frONT= path1/Path2/file.ext ": {MonId.direct: [monitor_file(
                                                                       file_path="path1/Path2/file.ext",
                                                                       detector_type=DetectorType.HAB)]}
                          }

        invalid_settings = {"MON/DIRECT= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/DIRECT /path1/Path2/file.ext ": RuntimeError,
                            "MON/DIRECT=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_flat_files_are_parsed_correctly(self):
        valid_settings = {"MON/FLat  = C:\path1\Path2\file.ext ": {MonId.flat: monitor_file(
                                                                   file_path="C:/path1/Path2/file.ext",
                                                                   detector_type=DetectorType.LAB)},
                          "MON/ flAt  = filE.Ext ": {MonId.flat: monitor_file(file_path="filE.Ext",
                                                     detector_type=DetectorType.LAB)},
                          "MON/flAT= \path1\Path2\file.ext ": {MonId.flat: monitor_file(
                                                               file_path="/path1/Path2/file.ext",
                                                               detector_type=DetectorType.LAB)},
                          "MON/FLat= /path1/Path2/file.ext ": {MonId.flat: monitor_file(
                                                               file_path="/path1/Path2/file.ext",
                                                               detector_type=DetectorType.LAB)},
                          "MON/FLat/ rear= /path1/Path2/file.ext ": {MonId.flat: monitor_file(
                                                                     file_path="/path1/Path2/file.ext",
                                                                     detector_type=DetectorType.LAB)},
                          "MON/FLat/ frONT= path1/Path2/file.ext ": {MonId.flat: monitor_file(
                                                                     file_path="path1/Path2/file.ext",
                                                                     detector_type=DetectorType.HAB)}}

        invalid_settings = {"MON/DIRECT= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/DIRECT /path1/Path2/file.ext ": RuntimeError,
                            "MON/DIRECT=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_hab_files_are_parsed_correctly(self):
        valid_settings = {"MON/HAB  = C:\path1\Path2\file.ext ": {MonId.direct: [monitor_file(
                                                                                file_path="C:/path1/Path2/file.ext",
                                                                                detector_type=DetectorType.HAB)]},
                          "MON/ hAB  = filE.Ext ": {MonId.direct: [monitor_file(
                                                                   file_path="filE.Ext",
                                                                   detector_type=DetectorType.HAB)]},
                          "MON/HAb= \path1\Path2\file.ext ": {MonId.direct: [monitor_file(
                                                              file_path="/path1/Path2/file.ext",
                                                              detector_type=DetectorType.HAB)]},
                          "MON/hAB= /path1/Path2/file.ext ": {MonId.direct: [monitor_file(
                                                              file_path="/path1/Path2/file.ext",
                                                              detector_type=DetectorType.HAB)]}}
        invalid_settings = {"MON/HAB= /path1/ Path2/file.ext ": RuntimeError,
                            "MON/hAB /path1/Path2/file.ext ": RuntimeError,
                            "MON/HAB=/path1/Path2/file ": RuntimeError}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_hab_files_are_parsed_correctly2(self):
        valid_settings = {"MON/Spectrum = 123 ": {MonId.spectrum: monitor_spectrum(spectrum=123, is_trans=False,
                                                                                   interpolate=False)},
                          "MON/trans/Spectrum = 123 ": {MonId.spectrum: monitor_spectrum(spectrum=123, is_trans=True,
                                                                                         interpolate=False)},
                          "MON/trans/Spectrum = 123 /  interpolate": {MonId.spectrum: monitor_spectrum(spectrum=123,
                                                                      is_trans=True, interpolate=True)},
                          "MON/Spectrum = 123 /  interpolate": {MonId.spectrum:  monitor_spectrum(spectrum=123,
                                                                                                  is_trans=False,
                                                                                                  interpolate=True)}}
        invalid_settings = {}

        mon_parser = MonParser()
        do_test(mon_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class PrintParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(PrintParser.get_type(), "PRINT")

    def test_that_print_is_parsed_correctly(self):
        valid_settings = {"PRINT OdlfP slsk 23lksdl2 34l": {PrintId.print_line: "OdlfP slsk 23lksdl2 34l"},
                          "PRiNt OdlfP slsk 23lksdl2 34l": {PrintId.print_line: "OdlfP slsk 23lksdl2 34l"},
                          "  PRINT Loaded: USER_LOQ_174J, 12/03/18, Xuzhi (Lu), 12mm, Sample Changer, Banjo cells":
                          {PrintId.print_line: "Loaded: USER_LOQ_174J, 12/03/18, Xuzhi (Lu), 12mm, Sample Changer, Banjo cells"}
                          }

        invalid_settings = {"j PRINT OdlfP slsk 23lksdl2 34l ": RuntimeError,}

        print_parser = PrintParser()
        do_test(print_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class BackParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(BackParser.get_type(), "BACK")

    def test_that_all_monitors_is_parsed_correctly(self):
        valid_settings = {"BACK / MON /times  123 34": {BackId.all_monitors: range_entry(start=123, stop=34)}}

        invalid_settings = {}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_single_monitors_is_parsed_correctly(self):
        valid_settings = {"BACK / M3 /times  123 34": {BackId.single_monitors: back_single_monitor_entry(monitor=3,
                                                                                                         start=123,
                                                                                                         stop=34)},
                          "BACK / M3 123 34": {BackId.single_monitors: back_single_monitor_entry(monitor=3,
                                                                                                 start=123,
                                                                                                 stop=34)}}

        invalid_settings = {"BACK / M 123 34": RuntimeError,
                            "BACK / M3 123": RuntimeError}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_off_is_parsed_correctly(self):
        valid_settings = {"BACK / M3 /OFF": {BackId.monitor_off: 3}}

        invalid_settings = {"BACK / M /OFF": RuntimeError}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)

    def test_that_trans_mon_is_parsed_correctly(self):
        valid_settings = {"BACK / TRANS  123 344": {BackId.trans:  range_entry(start=123, stop=344)},
                          "BACK / tranS 123 34": {BackId.trans: range_entry(start=123, stop=34)}}

        invalid_settings = {"BACK / Trans / 123 34": RuntimeError,
                            "BACK / trans 123": RuntimeError}

        back_parser = BackParser()
        do_test(back_parser, valid_settings, invalid_settings, self.assertTrue, self.assertRaises)


class SANS2DParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(SANS2DParser.get_type(), "SANS2D")

    def test_that_sans2d_is_parsed_correctly(self):
        sans_2d_parser = SANS2DParser()
        result = sans_2d_parser.parse_line("SANS2D ")
        self.assertNotEqual(result, None)
        self.assertTrue(not result)


class LOQParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(LOQParser.get_type(), "LOQ")

    def test_that_loq_is_parsed_correctly(self):
        loq_parser = LOQParser()
        result = loq_parser.parse_line("LOQ ")
        self.assertNotEqual(result, None)
        self.assertTrue(not result)


class LARMORParserTest(unittest.TestCase):
    def test_that_gets_type(self):
        self.assertTrue(LARMORParser.get_type(), "LARMOR")

    def test_that_loq_is_parsed_correctly(self):
        loq_parser = LARMORParser()
        result = loq_parser.parse_line("LARMOR ")
        self.assertNotEqual(result, None)
        self.assertTrue(not result)


class UserFileParserTest(unittest.TestCase):
    def test_that_correct_parser_is_selected_(self):
        # Arrange
        user_file_parser = UserFileParser()

        # DetParser
        result = user_file_parser.parse_line(" DET/CoRR/FRONT/ SidE -957")
        assert_valid_result(result, {DetectorId.correction_translation: single_entry_with_detector(entry=-957,
                                     detector_type=DetectorType.HAB)}, self.assertTrue)

        # LimitParser
        result = user_file_parser.parse_line("l/Q/WCUT 234.4")
        assert_valid_result(result, {LimitsId.wavelength_cut: 234.4}, self.assertTrue)

        # MaskParser
        result = user_file_parser.parse_line("MASK S 12  ")
        assert_valid_result(result, {MaskId.single_spectrum_mask: 12}, self.assertTrue)

        # SampleParser
        result = user_file_parser.parse_line("SAMPLE /Offset 234.5")
        assert_valid_result(result, {SampleId.offset: 234.5}, self.assertTrue)

        # TransParser
        result = user_file_parser.parse_line("TRANS / radius  =23")
        assert_valid_result(result, {TransId.radius: 23}, self.assertTrue)

        # TubeCalibFileParser
        result = user_file_parser.parse_line("TUBECALIbfile= calib_file.nxs")
        assert_valid_result(result, {TubeCalibrationFileId.file: "calib_file.nxs"}, self.assertTrue)

        # QResolutionParser
        result = user_file_parser.parse_line("QRESOL/ON")
        assert_valid_result(result, {QResolutionId.on: True}, self.assertTrue)

        # FitParser
        result = user_file_parser.parse_line("FIT/TRANS/Straight 123 3556")
        assert_valid_result(result, {FitId.general: fit_general(start=123, stop=3556, fit_type=FitType.Linear,
                                                                data_type=None, polynomial_order=0)},
                            self.assertTrue)

        # GravityParser
        result = user_file_parser.parse_line("Gravity/LExtra =23.5")
        assert_valid_result(result, {GravityId.extra_length: 23.5}, self.assertTrue)

        # MaskFileParser
        result = user_file_parser.parse_line("MaskFile= test.xml,   testKsdk2.xml,tesetlskd.xml")
        assert_valid_result(result, {MaskId.file: ["test.xml", "testKsdk2.xml", "tesetlskd.xml"]},
                            self.assertTrue)

        # MonParser
        result = user_file_parser.parse_line("MON/length= 23.5 34")
        assert_valid_result(result, {MonId.length: monitor_length(length=23.5, spectrum=34, interpolate=False)},
                            self.assertTrue)

        # PrintParser
        result = user_file_parser.parse_line("PRINT OdlfP slsk 23lksdl2 34l")
        assert_valid_result(result, {PrintId.print_line: "OdlfP slsk 23lksdl2 34l"}, self.assertTrue)

        # BackParser
        result = user_file_parser.parse_line("BACK / M3 /OFF")
        assert_valid_result(result, {BackId.monitor_off: 3}, self.assertTrue)

        # Instrument parser
        result = user_file_parser.parse_line("SANS2D")
        assert_valid_result(result, {DetectorId.instrument: SANSInstrument.SANS2D}, self.assertTrue)

        # Instrument parser - whitespace
        result = user_file_parser.parse_line("     ZOOM      ")
        assert_valid_result(result, {DetectorId.instrument: SANSInstrument.ZOOM}, self.assertTrue)

    def test_that_non_existent_parser_throws(self):
        # Arrange
        user_file_parser = UserFileParser()

        # Act + Assert
        self.assertRaises(ValueError, user_file_parser.parse_line, "DetT/DKDK/ 23 23")


if __name__ == "__main__":
    unittest.main()
