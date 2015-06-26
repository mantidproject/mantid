import unittest
import mantid
import isis_instrument as instruments
import ISISCommandInterface as command_iface
from reducer_singleton import ReductionSingleton
import isis_reduction_steps as reduction_steps


class SANSReductionStepsUserFileTest(unittest.TestCase):
    def test_parse_line_for_back_trans(self):
        # Arrange
        start = 1000
        end = 2000
        line = 'BACK/TRANS'+ str(start) +' ' + str(end)
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line = line, reducer = ReductionSingleton())
        start_TOF_ROI, end_TOF_ROI = ReductionSingleton().inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(start, start_TOF_ROI, 'The start time should be 1000 for ROI.')
        self.assertEqual(end, end_TOF_ROI, 'The end time should be 2000 for ROI.')

    def test_parse_line_for_back_trans_does_not_set_for_single_times(self):
        # Arrange
        start = 1000
        line = 'BACK/TRANS'+ str(start)
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line = line, reducer = ReductionSingleton())
        start_TOF_ROI, end_TOF_ROI = ReductionSingleton().inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(None, start_TOF_ROI, 'The start time should not have been set')
        self.assertEqual(None, end_TOF_ROI, 'The end time should not have been set')

if __name__ == "__main__":
    unittest.main()
