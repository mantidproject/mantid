from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.save import (get_save_builder)
from sans.state.data import (get_data_builder)
from sans.common.enums import (SANSFacility, SaveType)


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
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_save_builder(data_info)
        self.assertTrue(builder)

        user_specified_output_name = "test_file_name"
        zero_free_correction = True
        file_format = [SaveType.Nexus, SaveType.CanSAS]

        builder.set_user_specified_output_name(user_specified_output_name)
        builder.set_zero_free_correction(zero_free_correction)
        builder.set_file_format(file_format)
        state = builder.build()

        # Assert
        self.assertTrue(state.user_specified_output_name == user_specified_output_name)
        self.assertTrue(state.zero_free_correction == zero_free_correction)
        self.assertTrue(state.file_format == file_format)


if __name__ == '__main__':
    unittest.main()