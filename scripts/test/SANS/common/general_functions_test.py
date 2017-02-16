from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from mantid.kernel import (V3D, Quat)
from sans.common.general_functions import (quaternion_to_angle_and_axis, create_unmanaged_algorithm, add_to_sample_log)


class SANSFunctionsTest(unittest.TestCase):
    @staticmethod
    def _create_sample_workspace():
        sample_name = "CreateSampleWorkspace"
        sample_options = {"OutputWorkspace": "dummy"}
        sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
        sample_alg.execute()
        return sample_alg.getProperty("OutputWorkspace").value

    def _do_test_quaternion(self, angle, axis, expected_axis=None):
        # Act
        quaternion = Quat(angle, axis)
        converted_angle, converted_axis = quaternion_to_angle_and_axis(quaternion)

        # Assert
        if expected_axis is not None:
            axis = expected_axis
        self.assertAlmostEqual(angle, converted_angle)
        self.assertAlmostEqual(axis[0], converted_axis[0])
        self.assertAlmostEqual(axis[1], converted_axis[1])
        self.assertAlmostEqual(axis[2], converted_axis[2])

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_regular(self):
        # Arrange
        angle = 23.0
        axis = V3D(0.0, 1.0, 0.0)
        self._do_test_quaternion(angle, axis)

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_0_degree(self):
        # Arrange
        angle = 0.0
        axis = V3D(1.0, 0.0, 0.0)
        # There shouldn't be an axis for angle 0
        expected_axis = V3D(0.0, 0.0, 0.0)
        self._do_test_quaternion(angle, axis, expected_axis)

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_180_degree(self):
        # Arrange
        angle = 180.0
        axis = V3D(0.0, 1.0, 0.0)
        # There shouldn't be an axis for angle 0
        self._do_test_quaternion(angle, axis)

    def test_that_sample_log_is_added(self):
        # Arrange
        workspace = SANSFunctionsTest._create_sample_workspace()
        log_name = "TestName"
        log_value = "TestValue"
        log_type = "String"

        # Act
        add_to_sample_log(workspace, log_name, log_value, log_type)

        # Assert
        run = workspace.run()
        self.assertTrue(run.hasProperty(log_name))
        self.assertTrue(run.getProperty(log_name).value == log_value)

    def test_that_sample_log_raises_for_non_string_type_arguments(self):
        # Arrange
        workspace = SANSFunctionsTest._create_sample_workspace()
        log_name = "TestName"
        log_value = 123
        log_type = "String"

        # Act + Assert
        try:
            add_to_sample_log(workspace, log_name, log_value, log_type)
            did_raise = False
        except TypeError:
            did_raise = True
        self.assertTrue(did_raise)

    def test_that_sample_log_raises_for_wrong_type_selection(self):
        # Arrange
        workspace = SANSFunctionsTest._create_sample_workspace()
        log_name = "TestName"
        log_value = "test"
        log_type = "sdfsdfsdf"

        # Act + Assert
        try:
            add_to_sample_log(workspace, log_name, log_value, log_type)
            did_raise = False
        except ValueError:
            did_raise = True
        self.assertTrue(did_raise)


if __name__ == '__main__':
    unittest.main()
