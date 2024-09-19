# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from SANS.sans.common.enums import CanonicalCoordinates, SANSFacility, DetectorType, SANSInstrument
from SANS.sans.state.StateObjects.StateData import get_data_builder
from SANS.sans.state.StateObjects.StateMoveDetectors import (
    StateMoveLOQ,
    StateMoveSANS2D,
    StateMoveLARMOR,
    StateMoveZOOM,
    StateMove,
    StateMoveDetectors,
    get_move_builder,
)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateMoveWorkspaceTest(unittest.TestCase):
    def test_that_general_isis_default_values_are_set_up(self):
        state = StateMove()
        state.detectors = {DetectorType.LAB.value: StateMoveDetectors(), DetectorType.HAB.value: StateMoveDetectors()}
        self.assertEqual(state.sample_offset, 0.0)
        self.assertEqual(state.sample_offset_direction, CanonicalCoordinates.Z)
        self.assertEqual(state.detectors[DetectorType.HAB.value].x_translation_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].y_translation_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].z_translation_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].rotation_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].side_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].radius_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].x_tilt_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].y_tilt_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].z_tilt_correction, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].sample_centre_pos1, 0.0)
        self.assertEqual(state.detectors[DetectorType.HAB.value].sample_centre_pos2, 0.0)


class StateMoveWorkspaceLOQTest(unittest.TestCase):
    def test_that_is_sans_state_move_object(self):
        state = StateMoveLOQ()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_LOQ_has_centre_position_set_up(self):
        state = StateMoveLOQ()
        self.assertEqual(state.center_position, 317.5 / 1000.0)


class StateMoveWorkspaceSANS2DTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveSANS2D()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_sans2d_has_default_values_set_up(self):
        # Arrange
        state = StateMoveSANS2D()
        self.assertEqual(state.hab_detector_radius, 306.0 / 1000.0)
        self.assertEqual(state.hab_detector_default_sd_m, 4.0)
        self.assertEqual(state.hab_detector_default_x_m, 1.1)
        self.assertEqual(state.lab_detector_default_sd_m, 4.0)
        self.assertEqual(state.hab_detector_x, 0.0)
        self.assertEqual(state.hab_detector_z, 0.0)
        self.assertEqual(state.hab_detector_rotation, 0.0)
        self.assertEqual(state.lab_detector_x, 0.0)
        self.assertEqual(state.lab_detector_z, 0.0)
        self.assertEqual(state.monitor_4_offset, 0.0)


class StateMoveWorkspaceLARMORTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveLARMOR()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_can_set_and_get_values(self):
        state = StateMoveLARMOR()
        self.assertEqual(state.bench_rotation, 0.0)


class StateMoveWorkspaceZOOMTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = StateMoveZOOM()
        self.assertTrue(isinstance(state, StateMove))

    def test_that_can_set_and_get_values(self):
        state = StateMoveZOOM()
        self.assertEqual(len(state.detectors), 1)


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateMoveBuilderTest(unittest.TestCase):
    def test_that_state_for_loq_can_be_built(self):
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
        self.assertEqual(state.center_position, value)
        self.assertEqual(state.detectors[DetectorType.HAB.value].x_translation_correction, value)
        self.assertEqual(state.detectors[DetectorType.LAB.value].sample_centre_pos1, value)

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
        self.assertEqual(state.detectors[DetectorType.HAB.value].x_translation_correction, value)

    def test_state_with_no_file_info_can_be_built(self):
        data_info = mock.NonCallableMock()
        data_info.instrument = SANSInstrument.LARMOR
        data_info.idf_file_path = None
        data_info.ipf_file_path = None
        # This will happen if the user has not selected a run number
        data_info.sample_scatter_run_number = None

        builder = get_move_builder(data_info)
        self.assertEqual(1.0, builder.conversion_value)

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
        self.assertEqual(state.detectors[DetectorType.LAB.value].x_translation_correction, value)

    def test_that_state_for_zoom_can_be_built(self):
        # TODO when data becomes available
        pass


if __name__ == "__main__":
    unittest.main()
