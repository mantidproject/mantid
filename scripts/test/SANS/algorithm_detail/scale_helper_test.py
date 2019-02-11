# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import math

from mantid.api import FrameworkManager

from sans.test_helper.test_director import TestDirector
from sans.algorithm_detail.scale_helpers import (DivideByVolumeFactory, DivideByVolumeISIS, NullDivideByVolume,
                                                 MultiplyByAbsoluteScaleFactory, MultiplyByAbsoluteScaleLOQ,
                                                 MultiplyByAbsoluteScaleISIS)
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.enums import (SampleShape, SANSFacility, DataType, SANSInstrument)
from sans.state.scale import get_scale_builder
from sans.state.data import get_data_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


class ScaleHelperTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _get_workspace(width=1.0, height=1.0, thickness=1.0, shape=1):
        sample_name = "CreateSampleWorkspace"
        sample_options = {"WorkspaceType": "Histogram",
                          "NumBanks": 1,
                          "BankPixelWidth": 1,
                          "OutputWorkspace": "test"}
        sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
        sample_alg.execute()
        workspace = sample_alg.getProperty("OutputWorkspace").value

        sample = workspace.sample()
        sample.setGeometryFlag(shape)
        sample.setThickness(thickness)
        sample.setWidth(width)
        sample.setHeight(height)
        return workspace

    def test_that_divide_strategy_is_selected_for_isis_instrument_and_is_not_can(self):
        # Arrange
        test_director = TestDirector()
        state_isis = test_director.construct()
        divide_factory = DivideByVolumeFactory()
        # Act
        divider = divide_factory.create_divide_by_volume(state_isis)
        # Arrange
        self.assertTrue(isinstance(divider, DivideByVolumeISIS))

    def test_that_divide_uses_settings_from_workspace(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024, height=8.0,
                                                   width=8.0, thickness=1.0, shape=SampleShape.Disc)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state = test_director.construct()

        divide_factory = DivideByVolumeFactory()
        divider = divide_factory.create_divide_by_volume(state)

        # Apply the settings from the SANS2D00022024 workspace
        width = 8.
        height = 8.
        thickness = 1.
        shape = 3

        workspace = ScaleHelperTest._get_workspace(width, height, thickness, shape)

        # Act
        output_workspace = divider.divide_by_volume(workspace, scale_state)

        # Assert
        expected_volume = thickness * math.pi * math.pow(width, 2) / 4.0
        expected_value = 0.3 / expected_volume
        data_y = output_workspace.dataY(0)
        self.assertEqual(data_y[0], expected_value)

    def test_that_divide_uses_settings_from_state_if_they_are_set(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        width = 10.
        height = 5.
        thickness = 2.
        scale_builder.set_shape(SampleShape.Disc)
        scale_builder.set_thickness(thickness)
        scale_builder.set_width(width)
        scale_builder.set_height(height)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state = test_director.construct()

        divide_factory = DivideByVolumeFactory()
        divider = divide_factory.create_divide_by_volume(state)

        workspace = ScaleHelperTest._get_workspace()

        # Act
        output_workspace = divider.divide_by_volume(workspace, scale_state)

        # Assert
        expected_volume = thickness * math.pi * math.pow(width, 2) / 4.0
        expected_value = 0.3 / expected_volume
        data_y = output_workspace.dataY(0)
        self.assertEqual(data_y[0], expected_value)

    def test_that_correct_scale_strategy_is_selected_for_non_loq_isis_instrument(self):
        # Arrange
        test_director = TestDirector()
        state_isis = test_director.construct()
        absolute_multiply_factory = MultiplyByAbsoluteScaleFactory()
        # Act
        multiplier = absolute_multiply_factory.create_multiply_by_absolute(state_isis)
        # Arrange
        self.assertTrue(isinstance(multiplier, MultiplyByAbsoluteScaleISIS))

    def test_that_correct_scale_strategy_is_selected_for_loq(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state_loq = test_director.construct()

        absolute_multiply_factory = MultiplyByAbsoluteScaleFactory()
        # Act
        multiplier = absolute_multiply_factory.create_multiply_by_absolute(state_loq)

        # Assert
        self.assertTrue(isinstance(multiplier, MultiplyByAbsoluteScaleLOQ))

    def test_that_correct_scale_strategy_is_selected_for_loq_2(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_builder.set_scale(2.4)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state_loq = test_director.construct()

        absolute_multiply_factory = MultiplyByAbsoluteScaleFactory()
        multiplier = absolute_multiply_factory.create_multiply_by_absolute(state_loq)

        workspace = self._get_workspace()

        # Act
        output_workspace = multiplier.multiply_by_absolute_scale(workspace, state_loq.scale)

        # Assert
        expected_value = 0.3 * 2.4 / math.pi * 100.
        data_y = output_workspace.dataY(0)
        self.assertEqual(data_y[0], expected_value)


if __name__ == '__main__':
    unittest.main()
