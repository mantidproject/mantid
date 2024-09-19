# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.common.enums import SANSFacility, SaveType, SANSInstrument
from SANS.sans.state.StateObjects.StateData import get_data_builder
from SANS.sans.state.StateObjects.StateSave import get_save_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# No tests required
# ----------------------------------------------------------------------------------------------------------------------


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_save_builder(data_info)
        self.assertTrue(builder)

        user_specified_output_name = "test_file_name"
        zero_free_correction = True
        file_format = [SaveType.NEXUS, SaveType.CAN_SAS]

        builder.set_user_specified_output_name(user_specified_output_name)
        builder.set_zero_free_correction(zero_free_correction)
        builder.set_file_format(file_format)
        state = builder.build()

        # Assert
        self.assertEqual(state.user_specified_output_name, user_specified_output_name)
        self.assertEqual(state.zero_free_correction, zero_free_correction)
        self.assertEqual(state.file_format, file_format)


if __name__ == "__main__":
    unittest.main()
