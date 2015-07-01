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
        ReductionSingleton().transmission_calculator.trans_spec = trans_spectrum
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
        command_iface.SetTransmissionMonitorSpectrum(trans_spec = trans_spectrum)
        # Assert
        self.assertEqual(trans_spectrum, command_iface.GetTransmissionMonitorSpectrum(), 'The transmission spectrum should be set to 4.')

    def test_setting_transmission_monitor_to_invalid_input_does_not_set(self):
        # Arrange
        trans_spectrum = 4
        trans_spectrum_invalid = '23434_yh'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrum(trans_spec = trans_spectrum)
        command_iface.SetTransmissionMonitorSpectrum(trans_spec = trans_spectrum_invalid)
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
        command_iface.SetTransmissionMonitorSpectrumShift(trans_spec_shift = trans_spectrum_shift)
        # Assert
        result = command_iface.GetTransmissionMonitorSpectrumShift()

        self.assertEqual(trans_spectrum_shift, command_iface.GetTransmissionMonitorSpectrumShift(), 'The transmission monitor shift should be set to -55.')

    def test_setting_shift_with_invalid_input(self):
        # Arrange
        trans_spectrum_shift = '-55_thg'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMonitorSpectrumShift(trans_spec_shift = trans_spectrum_shift)
        # Assert
        result = command_iface.GetTransmissionMonitorSpectrumShift()

        self.assertEqual(None, command_iface.GetTransmissionMonitorSpectrumShift(), 'The transmission monitor shift should be None.')

if __name__ == "__main__":
    unittest.main()
