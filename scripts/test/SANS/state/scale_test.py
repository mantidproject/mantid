# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.scale import get_scale_builder
from sans.state.data import get_data_builder
from sans.common.enums import (SANSFacility, SANSInstrument, SampleShape)
from sans.test_helper.file_information_mock import SANSFileInformationMock



# ----------------------------------------------------------------------------------------------------------------------
#  State
# No tests required for the current states
# ----------------------------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()


        # Act
        builder = get_scale_builder(data_info, file_information)
        self.assertTrue(builder)

        builder.set_scale(1.0)
        builder.set_shape(SampleShape.FlatPlate)
        builder.set_thickness(3.6)
        builder.set_width(3.7)
        builder.set_height(5.8)

        # Assert
        state = builder.build()
        self.assertTrue(state.shape is SampleShape.FlatPlate)
        self.assertEqual(state.scale,  1.0)
        self.assertEqual(state.thickness,  3.6)
        self.assertEqual(state.width,  3.7)
        self.assertEqual(state.height,  5.8)


if __name__ == '__main__':
    unittest.main()
