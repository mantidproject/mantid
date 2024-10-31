# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import FileFinder
from mantid.simpleapi import mtd, config
from testhelpers import illhelpers
import unittest
import ReflectometryILL_common as common


class ReflectometryILL_commonTest(unittest.TestCase):
    def setUp(self):
        # all tests will be run on D17 workspace mockup
        self._ws = illhelpers.create_poor_mans_d17_workspace()
        mtd.add("ws", self._ws)
        self._long_duration = 31.0  # has to be above 30
        self._short_duration = 29.0  # has to be below 30

        self._def_fac = config["default.facility"]
        self._def_inst = config["default.instrument"]
        self._data_dirs = config["datasearch.directories"]
        # set instrument and append datasearch directory
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config.appendDataSearchSubDir("ILL/D17/")

    def tearDown(self):
        mtd.clear()
        config["default.facility"] = self._def_fac
        config["default.instrument"] = self._def_inst
        config["datasearch.directories"] = self._data_dirs

    def testChopperOpeningAngleLongDurationD17(self):
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("duration", self._long_duration, "s", True)  # patch for missing sample log
        chopper_window = 2
        chopper1_phase = 1
        chopper2_phase = 3
        open_offset = 0.5
        test_value = chopper_window - (chopper2_phase - chopper1_phase) - open_offset
        run.addProperty("ChopperWindow", chopper_window, "degrees", True)  # patch for missing sample log
        run.addProperty("Chopper1.phase_average", chopper1_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("Chopper2.phase_average", chopper2_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("VirtualChopper.open_offset", open_offset, "degrees", True)  # patch for missing sample log

        chopper_opening_angle = common.chopper_opening_angle(run, instrument)
        self.assertEqual(chopper_opening_angle, test_value)

    def testChopperOpeningAngleShortDurationD17(self):
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("duration", self._short_duration, "s", True)  # patch for missing sample log
        chopper_window = 2
        chopper1_phase = 1
        chopper2_phase = 3
        open_offset = 0.5
        test_value = chopper_window - (chopper2_phase - chopper1_phase) - open_offset
        run.addProperty("ChopperWindow", chopper_window, "degrees", True)  # patch for missing sample log
        run.addProperty("Chopper1.phase", chopper1_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("Chopper2.phase", chopper2_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("VirtualChopper.open_offset", open_offset, "degrees", True)  # patch for missing sample log

        chopper_opening_angle = common.chopper_opening_angle(run, instrument)
        self.assertEqual(chopper_opening_angle, test_value)

    def testChopperOpeningAngleFIGARO(self):
        self._ws = illhelpers.create_empty_figaro_workspace()
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("duration", self._short_duration, "s", True)  # patch for missing sample log
        chopper1_phase = 1
        chopper2_phase = 3
        open_offset = 0.5
        test_value = 45.0 - (chopper2_phase - chopper1_phase) - open_offset
        run.addProperty("ChopperSetting.firstChopper", 1, "degrees", True)  # patch for missing sample log
        run.addProperty("ChopperSetting.secondChopper", 2, "degrees", True)  # patch for missing sample log
        run.addProperty("chopper1.phase", chopper1_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("chopper2.phase", chopper2_phase, "degrees", True)  # patch for missing sample log
        run.addProperty("CollAngle.openOffset", open_offset, "degrees", True)  # patch for missing sample log

        chopper_opening_angle = common.chopper_opening_angle(run, instrument)
        self.assertEqual(chopper_opening_angle, test_value)

    def testChopperPairDistanceD17(self):
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        test_value = 5e-3
        run.addProperty("Distance.ChopperGap", float(test_value), "m", True)
        chopper_gap = common.chopper_pair_distance(run, instrument)
        self.assertEqual(chopper_gap, test_value)

    def testChopperPairDistanceFIGARO(self):
        self._ws = illhelpers.create_empty_figaro_workspace()
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        test_value = 5
        run.addProperty("ChopperSetting.distSeparationChopperPair", float(test_value), "mm", True)
        chopper_gap = common.chopper_pair_distance(run, instrument)
        self.assertEqual(chopper_gap, test_value * 1e-3)  # method internally converts mm to m

    def testChopperSpeedShortDurationD17(self):
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("duration", self._short_duration, "s", True)  # patch for missing sample log
        test_value = 11000
        run.addProperty("Chopper1.rotation_speed", float(test_value), "Hz", True)  # patch for missing sample log
        chopper_speed = common.chopper_speed(run, instrument)
        self.assertEqual(chopper_speed, test_value)

    def testChopperSpeedLongDurationD17(self):
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("duration", self._long_duration, "s", True)  # patch for missing sample log
        test_value = 12000
        run.addProperty("Chopper1.speed_average", float(test_value), "Hz", True)
        chopper_speed = common.chopper_speed(run, instrument)
        self.assertEqual(chopper_speed, test_value)

    def testChopperSpeedLongDurationFIGARO(self):
        self._ws = illhelpers.create_empty_figaro_workspace()
        instrument = self._ws.getInstrument()
        run = self._ws.getRun()
        run.addProperty("ChopperSetting.firstChopper", 1, "", True)  # patch for missing sample log
        test_value = 12000
        run.addProperty("chopper1.rotation_speed", float(test_value), "Hz", True)
        chopper_speed = common.chopper_speed(run, instrument)
        self.assertEqual(chopper_speed, test_value)

    def testDeflectionAngle(self):
        run = self._ws.getRun()
        test_value = 3.14159
        run.addProperty("CollAngle.actual_coll_angle", float(test_value), "radians", True)
        deflection_angle = common.deflection_angle(run)
        self.assertEqual(deflection_angle, test_value)

    def testDetectorAngle(self):
        detector_angle = common.detector_angle(FileFinder.getFullPath("397812.nxs"))
        self.assertAlmostEqual(detector_angle, 1.862, delta=1e-3)

    def testDetectorAngleNonExistingFile(self):
        self.assertRaisesRegex(RuntimeError, "Cannot load file wrong.nxs.", common.detector_angle, "wrong.nxs")

    def testDetectorResolution(self):
        detector_resolution = common.detector_resolution()
        self.assertEqual(detector_resolution, 0.0022)

    def testInstrumentName(self):
        instrument_name = common.instrument_name(self._ws)
        self.assertEqual(instrument_name, "D17")

    def testInstrumentNameIncorrectInstrument(self):
        incorrect_ws = illhelpers.create_poor_mans_in5_workspace(0.0, illhelpers.default_test_detectors)
        self.assertRaisesRegex(
            RuntimeError, "Unrecognized instrument IN5. Only D17 and FIGARO are supported.", common.instrument_name, incorrect_ws
        )

    def testPixelSizeD17(self):
        pixel_size = common.pixel_size("D17")
        self.assertEqual(pixel_size, 0.001195)

    def testPixelSizeFIGARO(self):
        pixel_size = common.pixel_size("FIGARO")
        self.assertEqual(pixel_size, 0.0012)

    def testSampleAngle(self):
        sample_angle = common.sample_angle(FileFinder.getFullPath("397812.nxs"))
        self.assertAlmostEqual(sample_angle, 0.002, delta=1e-3)

    def testSampleAngleNonExistingFile(self):
        self.assertRaisesRegex(RuntimeError, "Cannot load file wrong.nxs.", common.sample_angle, "wrong.nxs")

    def testSlitSizeLogEntryD17(self):
        slit_log_entry = common.slit_size_log_entry("D17", 1)
        self.assertEqual(slit_log_entry, "VirtualSlitAxis.s2w_actual_width")

    def testSlitSizeLogEntryFIGARO(self):
        slit_log_entry = common.slit_size_log_entry("FIGARO", 2)
        self.assertEqual(slit_log_entry, "VirtualSlitAxis.S3H_actual_height")

    def testSlitSizeLogEntryIncorrectSlit(self):
        args = {"instr_name": "_", "slit_number": 3}
        self.assertRaisesRegex(RuntimeError, "Slit number out of range.", common.slit_size_log_entry, **args)

    def testSlitSizes(self):
        illhelpers.add_slit_configuration_D17(self._ws, 0.03, 0.02)
        run = self._ws.run()
        instr_name = common.instrument_name(self._ws)
        slit2_width = run.get(common.slit_size_log_entry(instr_name, 1))
        slit3_width = run.get(common.slit_size_log_entry(instr_name, 2))
        common.slit_sizes(self._ws)
        self.assertEqual(slit2_width.value, run.getProperty(common.SampleLogs.SLIT2WIDTH).value)
        self.assertEqual(slit3_width.value, run.getProperty(common.SampleLogs.SLIT3WIDTH).value)


if __name__ == "__main__":
    unittest.main()
