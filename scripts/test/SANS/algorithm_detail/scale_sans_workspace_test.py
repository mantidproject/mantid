# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import unittest

from mantid.simpleapi import CreateSampleWorkspace
from sans.algorithm_detail.scale_sans_workspace import scale_workspace, _divide_by_sample_volume, _multiply_by_abs_scale
from sans.common.enums import SANSFacility, SampleShape, SANSInstrument, DetectorType
from sans.state.StateObjects.StateData import get_data_builder
from sans.state.StateObjects.StateScale import get_scale_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock
from sans.test_helper.test_director import TestDirector


class SANSScaleTest(unittest.TestCase):
    @staticmethod
    def _get_workspace():
        workspace = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, BankPixelWidth=1)
        return workspace

    @staticmethod
    def _set_sample_geometry(workspace, width=1.0, height=1.0, thickness=1.0, shape=1):
        sample = workspace.sample()
        sample.setGeometryFlag(shape)
        sample.setThickness(thickness)
        sample.setWidth(width)
        sample.setHeight(height)

    @staticmethod
    def _get_sample_state(width, height, thickness, shape, scale):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_builder.set_rear_scale(scale)
        scale_builder.set_thickness(thickness)
        scale_builder.set_width(width)
        scale_builder.set_height(height)
        scale_builder.set_shape(shape)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        return test_director.construct()

    def test_that__workspace_scaled_correctly(self):
        width = 1.0
        height = 2.0
        rear_scale = 7.2
        state = self._get_sample_state(width=width, height=height, thickness=3.0, scale=rear_scale, shape=SampleShape.CYLINDER)

        rear_workspace = self._get_workspace()
        rear_output_ws = scale_workspace(
            workspace=rear_workspace, instrument=SANSInstrument.LOQ, state_scale=state.scale, component=DetectorType.LAB
        )
        front_workspace = self._get_workspace()
        front_output_ws = scale_workspace(
            workspace=front_workspace, instrument=SANSInstrument.LOQ, state_scale=state.scale, component=DetectorType.HAB
        )

        # We have a LOQ data set, hence we need to divide by pi
        expected_value = 0.3 / (height * math.pi * math.pow(width, 2) / 4.0) * (rear_scale / math.pi) * 100.0
        data_y = rear_output_ws.dataY(0)
        self.assertAlmostEqual(data_y[0], expected_value, delta=1e-7)
        self.assertListEqual(list(rear_output_ws.dataY(0)), list(front_output_ws.dataY(0)))

    def test_that_divide_uses_settings_from_workspace(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(
            instrument=SANSInstrument.SANS2D, run_number=22024, height=8.0, width=8.0, thickness=1.0, shape=SampleShape.DISC
        )
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state = test_director.construct()

        # Apply the settings from the SANS2D00022024 workspace
        width = 8.0
        height = 8.0
        thickness = 1.0
        shape = 3

        workspace = self._get_workspace()
        self._set_sample_geometry(workspace=workspace, width=width, height=height, thickness=thickness, shape=shape)

        output_workspace = _divide_by_sample_volume(workspace=workspace, scale_info=state.scale)
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
        width = 10.0
        height = 5.0
        thickness = 2.0
        scale_builder.set_shape(SampleShape.DISC)
        scale_builder.set_thickness(thickness)
        scale_builder.set_width(width)
        scale_builder.set_height(height)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state = test_director.construct()

        workspace = self._get_workspace()

        output_workspace = _divide_by_sample_volume(workspace=workspace, scale_info=state.scale)

        expected_volume = thickness * math.pi * math.pow(width, 2) / 4.0
        expected_value = 0.3 / expected_volume
        data_y = output_workspace.dataY(0)
        self.assertEqual(data_y[0], expected_value)

    def test_that_correct_scale_strategy_is_selected_for_loq_2(self):
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        scale_builder = get_scale_builder(data_state, file_information)
        scale_builder.set_rear_scale(2.4)
        scale_builder.set_front_scale(4.8)
        scale_state = scale_builder.build()

        test_director = TestDirector()
        test_director.set_states(scale_state=scale_state, data_state=data_state)
        state_loq = test_director.construct()

        def run_and_test_output(detector, scale_factor):
            workspace = self._get_workspace()

            output_workspace = _multiply_by_abs_scale(
                instrument=SANSInstrument.LOQ, workspace=workspace, state_scale=state_loq.scale, component=detector
            )

            # Assert
            expected_value = 0.3 * scale_factor / math.pi * 100.0
            data_y = output_workspace.dataY(0)
            self.assertEqual(data_y[0], expected_value)

        run_and_test_output(DetectorType.LAB, 2.4)
        run_and_test_output(DetectorType.HAB, 4.8)


if __name__ == "__main__":
    unittest.main()
