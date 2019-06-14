# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.move import (StateMoveLOQ, StateMoveSANS2D, StateMoveLARMOR, StateMoveZOOM, StateMove,
                             StateMoveDetector, get_move_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (CanonicalCoordinates, SANSFacility, DetectorType, SANSInstrument)
from state_test_helper import assert_validate_error, assert_raises_nothing
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateMoveWorkspaceTest(unittest.TestCase):
    def test_that_raises_if_the_detector_name_is_not_set_up(self):
        state = StateMove()
        state.detectors = {DetectorType.to_string(DetectorType.LAB): StateMoveDetector(),
                           DetectorType.to_string(DetectorType.HAB): StateMoveDetector()}
        state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name = "test"
        state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name_short = "test"
        state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name_short = "test"
        assert_validate_error(self, ValueError, state)
        state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name = "test"
        assert_raises_nothing(self, state)

    def test_that_raises_if_the_short_detector_name_is_not_set_up(self):
        state = StateMove()
        state.detectors = {DetectorType.to_string(DetectorType.LAB): StateMoveDetector(),
                           DetectorType.to_string(DetectorType.HAB): StateMoveDetector()}
        state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name = "test"
        state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name = "test"
        state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name_short = "test"
        assert_validate_error(self, ValueError, state)
        state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name_short = "test"
        assert_raises_nothing(self, state)

    def test_that_general_isis_default_values_are_set_up(self):
        state = StateMove()
        state.detectors = {DetectorType.to_string(DetectorType.LAB): StateMoveDetector(),
                           DetectorType.to_string(DetectorType.HAB): StateMoveDetector()}
        self.assertEqual(state.sample_offset,  0.0)
        self.assertEqual(state.sample_offset_direction, CanonicalCoordinates.Z)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].x_translation_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].y_translation_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].z_translation_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].rotation_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].side_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].radius_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].x_tilt_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].y_tilt_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].z_tilt_correction,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos1,  0.0)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos2,  0.0)


class StateMoveWorkspaceLOQTest(unittest.TestCase):
    def test_that_is_sans_state_move_object(self):
        state = StateMoveLOQ()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_LOQ_has_centre_position_set_up(self):
        state = StateMoveLOQ()
        self.assertEqual(state.center_position,  317.5 / 1000.)
        self.assertEqual(state.monitor_names,  {})


class StateMoveWorkspaceSANS2DTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveSANS2D()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_sans2d_has_default_values_set_up(self):
        # Arrange
        state = StateMoveSANS2D()
        self.assertEqual(state.hab_detector_radius,  306.0/1000.)
        self.assertEqual(state.hab_detector_default_sd_m,  4.0)
        self.assertEqual(state.hab_detector_default_x_m,  1.1)
        self.assertEqual(state.lab_detector_default_sd_m,  4.0)
        self.assertEqual(state.hab_detector_x,  0.0)
        self.assertEqual(state.hab_detector_z,  0.0)
        self.assertEqual(state.hab_detector_rotation,  0.0)
        self.assertEqual(state.lab_detector_x,  0.0)
        self.assertEqual(state.lab_detector_z,  0.0)
        self.assertEqual(state.monitor_names,  {})
        self.assertEqual(state.monitor_n_offset,  0.0)


class StateMoveWorkspaceLARMORTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveLARMOR()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_can_set_and_get_values(self):
        state = StateMoveLARMOR()
        self.assertEqual(state.bench_rotation,  0.0)
        self.assertEqual(state.monitor_names,  {})


class StateMoveWorkspaceZOOMTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveZOOM()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_can_set_and_get_values(self):
        state = StateMoveZOOM()
        self.assertEqual(len(state.detectors),  1)
        self.assertEqual(state.monitor_names,  {})


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateMoveBuilderTest(unittest.TestCase):
    def test_that_state_for_loq_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_center_position(value)
        builder.set_HAB_x_translation_correction(value)
        builder.set_LAB_sample_centre_pos1(value)

        # Assert
        state = builder.build()
        self.assertEqual(state.center_position,  value)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].x_translation_correction,  value)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name_short,  "HAB")
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name,  "main-detector-bank")
        self.assertEqual(state.monitor_names[str(2)],  "monitor2")
        self.assertEqual(len(state.monitor_names),  2)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.LAB)].sample_centre_pos1,  value)

    def test_that_state_for_sans2d_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22048)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("SANS2D00022048")
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_HAB_x_translation_correction(value)

        # Assert
        state = builder.build()
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].x_translation_correction,  value)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.HAB)].detector_name_short,  "front")
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name,  "rear-detector")
        self.assertEqual(state.monitor_names[str(4)],  "monitor4")
        self.assertEqual(len(state.monitor_names),  4)

    def test_that_state_for_larmor_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LARMOR, run_number=2260)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LARMOR00002260")
        data_info = data_builder.build()

        # Act
        builder = get_move_builder(data_info)
        self.assertTrue(builder)
        value = 324.2
        builder.set_LAB_x_translation_correction(value)

        # Assert
        state = builder.build()
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.LAB)].x_translation_correction,  value)
        self.assertEqual(state.detectors[DetectorType.to_string(DetectorType.LAB)].detector_name,  "DetectorBench")
        self.assertTrue(DetectorType.to_string(DetectorType.HAB) not in state.detectors)
        self.assertEqual(state.monitor_names[str(5)],  "monitor5")
        self.assertEqual(len(state.monitor_names),  5)

    def test_that_state_for_zoom_can_be_built(self):
        # TODO when data becomes available
        pass


if __name__ == '__main__':
    unittest.main()
