# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.common.enums import SANSInstrument, DetectorType
from mantidqtinterfaces.sans_isis.gui_logic.models.beam_centre_model import BeamCentreModel


class BeamCentreModelTest(unittest.TestCase):
    def setUp(self):
        self.result = {"pos1": 300, "pos2": -300}
        self.centre_finder_instance = mock.MagicMock(return_value=self.result)
        self.SANSCentreFinder = mock.MagicMock(return_value=self.centre_finder_instance)
        self.beam_centre_model = BeamCentreModel()

    def test_that_model_initialises_with_correct_values(self):
        self.assertEqual(self.beam_centre_model.max_iterations, 10)
        self.assertEqual(self.beam_centre_model.r_min, 60)
        self.assertEqual(self.beam_centre_model.r_max, 280)
        self.assertEqual(self.beam_centre_model.left_right, True)
        self.assertEqual(self.beam_centre_model.up_down, True)
        self.assertEqual(self.beam_centre_model.tolerance, 0.0001251)
        self.assertEqual(self.beam_centre_model.rear_pos_1, "")
        self.assertEqual(self.beam_centre_model.rear_pos_2, "")
        self.assertEqual(self.beam_centre_model.front_pos_2, "")
        self.assertEqual(self.beam_centre_model.front_pos_1, "")
        self.assertEqual(self.beam_centre_model.COM, False)
        self.assertEqual(self.beam_centre_model.verbose, False)
        self.assertEqual(self.beam_centre_model.q_min, 0.01)
        self.assertEqual(self.beam_centre_model.q_max, 0.1)
        self.assertEqual(self.beam_centre_model.component, DetectorType.LAB)
        self.assertTrue(self.beam_centre_model.update_rear)
        self.assertTrue(self.beam_centre_model.update_front)

    def test_all_other_hardcoded_inst_values_taken(self):
        for inst in {SANSInstrument.NO_INSTRUMENT, SANSInstrument.SANS2D, SANSInstrument.ZOOM}:
            self.beam_centre_model.reset_inst_defaults(instrument=inst)
            self.assertEqual(60, self.beam_centre_model.r_min)
            self.assertEqual(280, self.beam_centre_model.r_max)

    def test_update_centre_positions_front_mode(self):
        expected_vals = {"pos1": 101.123, "pos2": 202.234}
        rear_vals_before = (self.beam_centre_model.rear_pos_1, self.beam_centre_model.rear_pos_2)
        self.beam_centre_model.component = DetectorType.HAB  # Where HAB == front
        self.beam_centre_model.update_centre_positions(expected_vals)

        # mm -> m scaling
        self.assertEqual(expected_vals["pos1"] * 1000, self.beam_centre_model.front_pos_1)
        self.assertEqual(expected_vals["pos2"] * 1000, self.beam_centre_model.front_pos_2)

        self.assertEqual(rear_vals_before[0], self.beam_centre_model.rear_pos_1)
        self.assertEqual(rear_vals_before[1], self.beam_centre_model.rear_pos_2)

    def test_update_centre_positions_rear_mode(self):
        expected_vals = {"pos1": 303.345, "pos2": 404.456}
        front_vals_before = (self.beam_centre_model.front_pos_1, self.beam_centre_model.front_pos_2)
        self.beam_centre_model.component = DetectorType.LAB  # Where LAB == rear
        self.beam_centre_model.update_centre_positions(expected_vals)

        # mm -> m scaling
        self.assertEqual(expected_vals["pos1"] * 1000, self.beam_centre_model.rear_pos_1)
        self.assertEqual(expected_vals["pos2"] * 1000, self.beam_centre_model.rear_pos_2)

        self.assertEqual(front_vals_before[0], self.beam_centre_model.front_pos_1)
        self.assertEqual(front_vals_before[1], self.beam_centre_model.front_pos_2)

    def test_loq_values_updated(self):
        self.beam_centre_model.reset_inst_defaults(SANSInstrument.LOQ)
        self.assertEqual(96, self.beam_centre_model.r_min)
        self.assertEqual(216, self.beam_centre_model.r_max)

    def test_that_can_update_model_values(self):
        self.beam_centre_model.r_max = 1.0
        self.assertEqual(self.beam_centre_model.r_max, 1.0)

    def test_beam_centre_scales_to_mills(self):
        self.assertIsNot(SANSInstrument.LARMOR, self.beam_centre_model.instrument)

        value_in_mm = 1200
        self.beam_centre_model.rear_pos_1 = value_in_mm
        self.beam_centre_model.front_pos_2 = value_in_mm * 2

        self.assertEqual(value_in_mm, self.beam_centre_model.rear_pos_1)
        self.assertEqual((value_in_mm * 2), self.beam_centre_model.front_pos_2)
        # Should internally be in m
        self.assertEqual((value_in_mm / 1000), self.beam_centre_model._rear_pos_1)
        self.assertEqual((value_in_mm * 2 / 1000), self.beam_centre_model._front_pos_2)

    def test_beam_centre_does_not_scale(self):
        self.beam_centre_model.instrument = SANSInstrument.LARMOR

        value_in_m = 1.2
        self.beam_centre_model.rear_pos_1 = value_in_m
        self.beam_centre_model.front_pos_1 = value_in_m * 2
        self.assertEqual(value_in_m, self.beam_centre_model.rear_pos_1)
        self.assertEqual(value_in_m, self.beam_centre_model._rear_pos_1)

        self.assertEqual((value_in_m * 2), self.beam_centre_model.front_pos_1)
        self.assertEqual((value_in_m * 2), self.beam_centre_model._front_pos_1)

    def test_instrument_is_set(self):
        for inst in SANSInstrument:
            self.beam_centre_model.reset_inst_defaults(inst)
            self.assertEqual(inst, self.beam_centre_model.instrument)

    def test_scaling_does_not_affect_non_rear_front_values(self):
        value_in_mm = 100  # 0.1 m

        for inst in [SANSInstrument.LARMOR, SANSInstrument.LOQ]:
            self.beam_centre_model.instrument = inst
            self.beam_centre_model.tolerance = value_in_mm
            self.assertEqual((value_in_mm / 1000), self.beam_centre_model._tolerance)
            self.assertEqual(self.beam_centre_model.tolerance, value_in_mm)

    def test_scaling_can_handle_non_float_types(self):
        self.beam_centre_model.instrument = SANSInstrument.NO_INSTRUMENT

        # When in doubt it should just forward the value as is
        self.beam_centre_model.rear_pos_1 = "a"
        self.assertEqual(self.beam_centre_model.rear_pos_1, "a")

    def test_scaling_ignores_zero_vals(self):
        self.beam_centre_model.lab_pos_1 = 0.0
        self.beam_centre_model.lab_pos_2 = 0.0
        self.beam_centre_model.hab_pos_1 = 0.0
        self.beam_centre_model.hab_pos_2 = 0.0

        self.assertEqual(0.0, self.beam_centre_model.lab_pos_1)
        self.assertEqual(0.0, self.beam_centre_model.lab_pos_2)
        self.assertEqual(0.0, self.beam_centre_model.hab_pos_1)
        self.assertEqual(0.0, self.beam_centre_model.hab_pos_2)


if __name__ == "__main__":
    unittest.main()
