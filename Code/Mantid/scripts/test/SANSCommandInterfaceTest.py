import unittest
import mantid
import isis_instrument as instruments
import ISISCommandInterface as command_iface
from reducer_singleton import ReductionSingleton
import isis_reduction_steps as reduction_steps


class SANSCommandInterfaceGetAndSetTransmissionSettings(unittest.TestCase):
    def test_that_gets_transmission_monitor(self):
        # Arrange
        trans_spectrum = 4
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().transmission_calculator.trans_mon = trans_spectrum
        # Act
        result = command_iface.GetTransmissionMonitorSpectrum()
        # Assert
        self.assertEqual(trans_spectrum, result, 'The transmission spectrum should be set to 4.')

    def test_setting_transmission_monitor_to_valid_input(self):
        # Arrange
        trans_spectrum = 4
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrum(trans_mon = trans_spectrum)
        # Assert
        self.assertEqual(trans_spectrum, command_iface.GetTransmissionMonitorSpectrum(), 'The transmission spectrum should be set to 4.')

    def test_setting_transmission_monitor_to_invalid_input_does_not_set(self):
        # Arrange
        trans_spectrum = 4
        trans_spectrum_invalid = '23434_yh'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrum(trans_mon = trans_spectrum)
        command_iface.SetTransmissionMonitorSpectrum(trans_mon = trans_spectrum_invalid)
        # Assert
        self.assertEqual(trans_spectrum, command_iface.GetTransmissionMonitorSpectrum(), 'The transmission spectrum should be set to 4.')

    def test_that_gets_transmission_monitor_shift(self):
        # Arrange
        trans_spectrum_shift = -55
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().get_instrument().monitor_4_offset = trans_spectrum_shift
        # Act
        result = command_iface.GetTransmissionMonitorSpectrumShift()
        # Assert
        self.assertEqual(trans_spectrum_shift, result, 'The transmission monitor shift should be set to -55.')

    def test_setting_shift_to_valid_value(self):
        # Arrange
        trans_spectrum_shift = -55.0
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrumShift(trans_mon_shift = trans_spectrum_shift)
        # Assert
        self.assertEqual(trans_spectrum_shift, command_iface.GetTransmissionMonitorSpectrumShift(), 'The transmission monitor shift should be set to -55.')

    def test_setting_shift_with_invalid_input(self):
        # Arrange
        trans_spectrum_shift = '-55_thg'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrumShift(trans_mon_shift = trans_spectrum_shift)
        # Assert
        self.assertEqual(None, command_iface.GetTransmissionMonitorSpectrumShift(), 'The transmission monitor shift should be None.')

    def test_that_gets_transmission_radius(self):
        # Arrange
        trans_radius = 23
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().transmission_calculator.radius = trans_radius
        # Act
        result = command_iface.GetTransmissionRadius()
        # Assert
        self.assertEqual(trans_radius, result, 'The transmission radius should be set to 23.')

    def test_setting_radius_to_valid_value(self):
        # Arrange
        trans_radius = 23
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionRadius(trans_radius = trans_radius)
        # Assert
        self.assertEqual(trans_radius, command_iface.GetTransmissionRadius(), 'The transmission radius should be set to 23.')

    def test_setting_radius_with_invalid_input(self):
        # Arrange
        trans_radius = '23_yh'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionRadius(trans_radius = trans_radius)
        # Assert
        self.assertEqual(None, command_iface.GetTransmissionRadius(), 'The transmission radius should be None.')

if __name__ == "__main__":
    unittest.main()
