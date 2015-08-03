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
        trans_radius = 23/1000
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().transmission_calculator.radius = trans_radius
        # Act
        result = command_iface.GetTransmissionRadiusInMM()
        # Assert
        self.assertEqual(trans_radius*1000, result, 'The transmission radius should be set to 23 mm.')

    def test_setting_radius_to_valid_value(self):
        # Arrange
        trans_radius = 23
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionRadiusInMM(trans_radius = trans_radius)
        # Assert
        self.assertEqual(trans_radius, command_iface.GetTransmissionRadiusInMM(), 'The transmission radius should be set to 23.')

    def test_setting_radius_with_invalid_input(self):
        # Arrange
        trans_radius = '23_yh'
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionRadiusInMM(trans_radius = trans_radius)
        # Assert
        self.assertEqual(None, command_iface.GetTransmissionRadiusInMM(), 'The transmission radius should be None.')



    def test_that_gets_non_empty_roi_files(self):
        # Arrange
        trans_roi_files = ['roi_file1.xml', 'roi_file2.xml']
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().transmission_calculator.roi_files = trans_roi_files
        # Act
        result = command_iface.GetTransmissionROI()
        # Assert
        self.assertEqual(trans_roi_files, result, 'The transmission roi should have two entries')

    def test_that_gets_None_for_empty_roi_files(self):
         # Arrange
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        result = command_iface.GetTransmissionROI()
        # Assert
        self.assertEqual(None, result, 'The transmission roi should be None')

    def test_setting_roi_file_for_valid_input(self):
         # Arrange
        trans_roi_files = ['file1.xml', 'file2.xml']
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionROI(trans_roi_files = trans_roi_files)
        # Assert
        result = ReductionSingleton().transmission_calculator.roi_files
        self.assertEqual(2, len(result), 'The transmission roi list should have two entries')
        self.assertEqual("file1.xml", result[0], 'The first file should be file1.xml')
        self.assertEqual("file2.xml", result[1], 'The second file should be file2.xml')

    def test_setting_roi_file_for_invalid_input(self):
         # Arrange
        trans_roi_files = ['file1g', 'file2.xml']
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionROI(trans_roi_files = trans_roi_files)
        # Assert
        self.assertEqual(0, len(ReductionSingleton().transmission_calculator.roi_files), 'The transmission roi list should be empty.')



    def test_that_gets_non_empty_mask_files(self):
        # Arrange
        trans_mask_files = ['mask_file1.xml', 'mask_file2.xml']
        command_iface.Clean()
        command_iface.SANS2D()
        ReductionSingleton().transmission_calculator.mask_files = trans_mask_files
        # Act
        result = command_iface.GetTransmissionMask()
        # Assert
        self.assertEqual(trans_mask_files, result, 'The transmission mask should have two entries')

    def test_that_gets_None_for_empty_mask_files(self):
         # Arrange
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        result = command_iface.GetTransmissionMask()
        # Assert
        self.assertEqual(None, result, 'The transmission mask should be None')

    def test_setting_mask_file_for_valid_input(self):
         # Arrange
        trans_mask_files = ['file1.xml', 'file2.xml']
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMask(trans_mask_files = trans_mask_files)
        # Assert
        result = ReductionSingleton().transmission_calculator.mask_files
        self.assertEqual(2, len(result), 'The transmission mask list should have two entries')
        self.assertEqual("file1.xml", result[0], 'The first file should be file1.xml')
        self.assertEqual("file2.xml", result[1], 'The second file should be file2.xml')

    def test_setting_mask_file_for_invalid_input(self):
         # Arrange
        trans_mask_files = " file1g,  file2.xml "
        command_iface.Clean()
        command_iface.SANS2D()
        # Act
        command_iface.SetTransmissionMask(trans_mask_files = trans_mask_files)
        # Assert
        self.assertEqual(0, len(ReductionSingleton().transmission_calculator.mask_files), 'The transmission mask list should be empty.')

if __name__ == "__main__":
    unittest.main()
