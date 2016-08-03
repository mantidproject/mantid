import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSEnumerations import SANSFacility


class SANSStateDataBuilderTest(unittest.TestCase):
    def test_that_data_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS

        # Act
        data_builder = get_data_builder(facility)

        data_builder.set_sample_scatter("test")
        data_builder.set_sample_scatter_period(3)
        data_state = data_builder.build()

        # # Assert
        self.assertTrue(data_state.sample_scatter == "test")
        self.assertTrue(data_state.sample_scatter_period == 3)
        self.assertTrue(data_state.sample_direct_period == 0)


if __name__ == '__main__':
    unittest.main()
