import unittest
import mantid
import centre_finder as cf
import ISISCommandInterface as command_iface
from reducer_singleton import ReductionSingleton
import isis_reduction_steps as reduction_steps

class SANSBeamCentrePositionUpdater(unittest.TestCase):
    def test_that_find_ALL_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.createBeamCentrePositionUpdater(cf.FindDirectionEnum.ALL)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.incrementPosition(x, y, x_step, y_step)
        x_out, y_out = position_updater.produceFinalPosition(x_new, x, y_new, y)

        # Assert
        x_expected = 1.1
        y_expected = 2.2
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")
        self.assertEqual(x_expected, x_out, "The x output should not be the initial value.")
        self.assertEqual(y_expected, y_out, "The y output should not be the initial value.")

    def test_that_find_LEFTRIGHT_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.createBeamCentrePositionUpdater(cf.FindDirectionEnum.LEFT_RIGHT)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.incrementPosition(x, y, x_step, y_step)
        x_out, y_out = position_updater.produceFinalPosition(x_new, x, y_new, y)

        # Assert
        x_expected = 1.1
        y_expected = 2.0
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")
        self.assertEqual(x_expected, x_out, "The x output should not be the initial value.")
        self.assertEqual(y_expected, y_out, "The y output should not be the initial value.")

    def test_that_find_UPPDOWN_produces_correct_increment(self):
        # Arrange
        fac = cf.BeamCentrePositionUpdaterFactory()
        position_updater = fac.createBeamCentrePositionUpdater(cf.FindDirectionEnum.UP_DOWN)
        x = 1.0
        y = 2.0
        x_step = 0.1
        y_step = 0.2

        # Act
        x_new, y_new = position_updater.incrementPosition(x, y, x_step, y_step)
        x_out, y_out = position_updater.produceFinalPosition(x_new, x, y_new, y)

        # Assert
        x_expected = 1.0
        y_expected = 2.2
        self.assertEqual(x_expected, x_new, "The x value should have been incremented.")
        self.assertEqual(y_expected, y_new, "The y value should have been incremented.")
        self.assertEqual(x_expected, x_out, "The x output should not be the initial value.")
        self.assertEqual(y_expected, y_out, "The y output should not be the initial value.")





if __name__ == "__main__":
    unittest.main()
