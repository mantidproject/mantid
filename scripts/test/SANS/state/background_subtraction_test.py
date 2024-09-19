# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.common.enums import SANSFacility
from SANS.sans.state.StateObjects.StateData import get_data_builder
from SANS.sans.state.StateObjects.StateBackgroundSubtraction import StateBackgroundSubtraction, get_background_subtraction_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateBackgroundSubtractionTest(unittest.TestCase):
    def test_that_raises_when_only_bgws_is_set(self):
        state = StateBackgroundSubtraction()
        state.workspace = "testws"
        with self.assertRaises(ValueError):
            state.validate()

    def test_that_raises_when_only_scale_factor_is_set(self):
        state = StateBackgroundSubtraction()
        state.scale_factor = 1.08
        with self.assertRaises(ValueError):
            state.validate()

    def test_that_valid_when_none_set(self):
        state = StateBackgroundSubtraction()
        state.validate()

    def test_that_valid_when_both_set(self):
        state = StateBackgroundSubtraction()
        state.scale_factor = 1.08
        state.workspace = "testws"
        state.validate()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateBackgroundSubtractionBuilderTest(unittest.TestCase):
    def test_that_background_subtraction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_background_subtraction_builder(data_info)
        self.assertTrue(builder)

        builder.set_workspace("test_ws")
        builder.set_scale_factor(1.17)

        # Assert
        state = builder.build()
        self.assertEqual(state.workspace, "test_ws")
        self.assertEqual(state.scale_factor, 1.17)


if __name__ == "__main__":
    unittest.main()
