# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from reducer_singleton import ReductionSingleton

import ISISCommandInterface as command_iface
import centre_finder as cf
from mantid.simpleapi import *


class SANSBeamCentrePositionUpdater(unittest.TestCase):
    def test_that_find_ALL_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.create_beam_centre_position_updater(cf.FindDirectionEnum.ALL)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.increment_position(x, y, x_step, y_step)

        # Assert
        x_expected = 1.1
        y_expected = 2.2
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")

    def test_that_find_LEFTRIGHT_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.create_beam_centre_position_updater(cf.FindDirectionEnum.LEFT_RIGHT)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.increment_position(x, y, x_step, y_step)

        # Assert
        x_expected = 1.1
        y_expected = 2.0
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")

    def test_that_find_UPPDOWN_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.create_beam_centre_position_updater(cf.FindDirectionEnum.UP_DOWN)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.increment_position(x, y, x_step, y_step)

        # Assert
        x_expected = 1.0
        y_expected = 2.2
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")


class TestPositionProvider(unittest.TestCase):
    workspace_name = "dummy_ws"

    class MockSample(object):
        """
        Mocking out the sample
        """

        def __init__(self, ws_name):
            super(TestPositionProvider.MockSample, self).__init__()
            self.wksp_name = ws_name

        def get_wksp_name(self):
            return self.wksp_name

    def _provide_reducer(self, is_larmor, is_new=True):
        """
        Provide a reducer with either Larmor or non-Larmor. If we have Larmor,
        then we want to be able to set the run number as well
        """
        command_iface.Clean()
        if is_larmor and is_new:
            command_iface.LARMOR()
            CreateSampleWorkspace(OutputWorkspace=self.workspace_name)
            AddSampleLog(Workspace=self.workspace_name, LogName="run_number", LogText="3000", LogType="Number")
            sample = self.MockSample(self.workspace_name)
            ReductionSingleton()._sample_run = sample
            return ReductionSingleton()

        elif is_larmor and not is_new:
            command_iface.LARMOR()
            CreateSampleWorkspace(OutputWorkspace=self.workspace_name)
            AddSampleLog(Workspace=self.workspace_name, LogName="run_number", LogText="1000", LogType="Number")
            sample = self.MockSample(self.workspace_name)
            ReductionSingleton()._sample_run = sample
            return ReductionSingleton()
        else:
            command_iface.LOQ()
            return ReductionSingleton()

    def _clean_up(self, workspace_name):
        if workspace_name in mtd.getObjectNames():
            mtd.remove(workspace_name)

    def test_that_XY_increment_provider_is_created_for_non_larmor(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        is_larmor = False
        reducer = self._provide_reducer(is_larmor)
        # Act
        factory = cf.PositionProviderFactory(
            increment_coord1=increment_coord1,
            increment_coord2=increment_coord2,
            tolerance=tolerance,
            position_type=cf.FindDirectionEnum.ALL,
        )
        provider = factory.create_position_provider(reducer=reducer)
        # Asssert
        self.assertTrue(isinstance(provider, cf.PositionProviderXY), "Should create a XY increment provider")
        # Clean up
        self._clean_up(self.workspace_name)

    def test_that_YAngle_increment_provider_is_created_for_larmor(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        is_larmor = True
        is_new = True
        reducer = self._provide_reducer(is_larmor, is_new)
        # Act
        factory = cf.PositionProviderFactory(
            increment_coord1=increment_coord1,
            increment_coord2=increment_coord2,
            tolerance=tolerance,
            position_type=cf.FindDirectionEnum.ALL,
        )
        provider = factory.create_position_provider(reducer=reducer)

        # Asssert
        self.assertTrue(isinstance(provider, cf.PositionProviderAngleY), "Should create a AngleY increment provider")
        # Clean up
        self._clean_up(self.workspace_name)

    def test_that_XY_increment_provider_is_created_for_old_larmor(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        is_larmor = True
        is_new = False
        reducer = self._provide_reducer(is_larmor, is_new)
        # Act
        factory = cf.PositionProviderFactory(
            increment_coord1=increment_coord1,
            increment_coord2=increment_coord2,
            tolerance=tolerance,
            position_type=cf.FindDirectionEnum.ALL,
        )
        provider = factory.create_position_provider(reducer=reducer)

        # Asssert
        self.assertTrue(isinstance(provider, cf.PositionProviderXY), "Should create a XY increment provider")
        # Clean up
        self._clean_up(self.workspace_name)

    def test_that_XY_increment_provider_halves_the_step(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        provider = cf.PositionProviderXY(increment_coord1, increment_coord2, tolerance)

        # Act and Assert
        self.assertEqual(increment_coord1, provider.get_increment_coord1())
        self.assertEqual(increment_coord2, provider.get_increment_coord2())

        provider.half_and_reverse_increment_coord1()
        provider.half_and_reverse_increment_coord2()

        self.assertEqual(-increment_coord1 / 2.0, provider.get_increment_coord1())
        self.assertEqual(-increment_coord2 / 2.0, provider.get_increment_coord2())

    def test_that_AngleY_increment_provider_halves_the_step(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        tolerance_angle = 33
        bench_rotation = 1
        coord1_scale_factor = 1
        provider = cf.PositionProviderAngleY(
            increment_coord1, increment_coord2, tolerance, tolerance_angle, bench_rotation, coord1_scale_factor
        )

        # Act and Assert
        self.assertEqual(increment_coord1, provider.get_increment_coord1())
        self.assertEqual(increment_coord2, provider.get_increment_coord2())

        provider.half_and_reverse_increment_coord1()
        provider.half_and_reverse_increment_coord2()

        self.assertEqual(-increment_coord1 / 2.0, provider.get_increment_coord1())
        self.assertEqual(-increment_coord2 / 2.0, provider.get_increment_coord2())

    def test_that_XY_increment_is_smaller_than_tolerance(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 200
        provider = cf.PositionProviderXY(increment_coord1, increment_coord2, tolerance)

        # Act
        is_smaller_coord1 = provider.is_coord1_increment_smaller_than_tolerance()
        is_smaller_coord2 = provider.is_coord2_increment_smaller_than_tolerance()

        # Assert
        self.assertTrue(is_smaller_coord1)
        self.assertTrue(is_smaller_coord2)

    def test_that_XY_increment_is_larger_than_tolerance(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 0.2
        provider = cf.PositionProviderXY(increment_coord1, increment_coord2, tolerance)

        # Act
        is_smaller_coord1 = provider.is_coord1_increment_smaller_than_tolerance()
        is_smaller_coord2 = provider.is_coord2_increment_smaller_than_tolerance()

        # Assert
        self.assertFalse(is_smaller_coord1)
        self.assertFalse(is_smaller_coord2)

    def test_that_AngleY_increment_is_smaller_than_tolerance(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 100
        tolerance_angle = 233
        bench_rotation = 1
        coord1_scale_factor = 1
        provider = cf.PositionProviderAngleY(
            increment_coord1, increment_coord2, tolerance, tolerance_angle, bench_rotation, coord1_scale_factor
        )

        # Act
        is_smaller_coord1 = provider.is_coord1_increment_smaller_than_tolerance()
        is_smaller_coord2 = provider.is_coord2_increment_smaller_than_tolerance()

        # Assert
        self.assertTrue(is_smaller_coord1)
        self.assertTrue(is_smaller_coord2)

    def test_that_AngleY_increment_is_larger_than_tolerance(self):
        # Arrange
        increment_coord1 = 1
        increment_coord2 = 2
        tolerance = 0.2
        tolerance_angle = 0.1
        bench_rotation = 1
        coord1_scale_factor = 1
        provider = cf.PositionProviderAngleY(
            increment_coord1, increment_coord2, tolerance, tolerance_angle, bench_rotation, coord1_scale_factor
        )

        # Act
        is_smaller_coord1 = provider.is_coord1_increment_smaller_than_tolerance()
        is_smaller_coord2 = provider.is_coord2_increment_smaller_than_tolerance()

        # Assert
        self.assertFalse(is_smaller_coord1)
        self.assertFalse(is_smaller_coord2)


if __name__ == "__main__":
    unittest.main()
