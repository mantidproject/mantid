# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from reducer_singleton import ReductionSingleton

import ISISCommandInterface as command_iface
import isis_reduction_steps as reduction_steps


class SANSReductionStepsUserFileTest(unittest.TestCase):
    def test_parse_line_for_back_trans(self):
        # Arrange
        start = 1000
        end = 2000
        line = "BACK/TRANS" + str(start) + " " + str(end)
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        start_TOF_ROI, end_TOF_ROI = ReductionSingleton().inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(start, start_TOF_ROI, "The start time should be 1000 for ROI.")
        self.assertEqual(end, end_TOF_ROI, "The end time should be 2000 for ROI.")

    def test_parse_line_for_back_trans_does_not_set_for_single_times(self):
        # Arrange
        start = 1000
        line = "BACK/TRANS" + str(start)
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        start_TOF_ROI, end_TOF_ROI = ReductionSingleton().inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(None, start_TOF_ROI, "The start time should not have been set")
        self.assertEqual(None, end_TOF_ROI, "The end time should not have been set")

    def test_can_parse_DET_OVERLAP_line(self):
        # Arrange
        line = "DET/OVERLAP 0.13 0.15"
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        merge_Range = ReductionSingleton().instrument.getDetector("FRONT").mergeRange

        # Assert
        self.assertEqual(merge_Range.q_min, 0.13, "The q_min should have been read in")
        self.assertEqual(merge_Range.q_max, 0.15, "The q_max should have been read in")
        self.assertEqual(merge_Range.q_merge_range, True, "q_merge_range should be true")

    def test_that_will_not_parse_DET_OVERLAP_with_no_subsequent_commands(self):
        # Arrange
        line = "DET/OVERLAP"
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        merge_Range = ReductionSingleton().instrument.getDetector("FRONT").mergeRange

        # Assert
        self.assertEqual(merge_Range.q_min, None, "The q_min should have been read in")
        self.assertEqual(merge_Range.q_max, None, "The q_max should have been read in")
        self.assertEqual(merge_Range.q_merge_range, False, "q_merge_range should be true")

    def test_that_will_not_parse_DET_OVERLAP_with_one_subsequent_commands(self):
        # Arrange
        line = "DET/OVERLAP 0.13"
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        merge_Range = ReductionSingleton().instrument.getDetector("FRONT").mergeRange

        # Assert
        self.assertEqual(merge_Range.q_min, None, "The q_min should have been read in")
        self.assertEqual(merge_Range.q_max, None, "The q_max should have been read in")
        self.assertEqual(merge_Range.q_merge_range, False, "q_merge_range should be true")

    def test_that_will_not_parse_DET_OVERLAP_with_three_subsequent_commands(self):
        # Arrange
        line = "DET/OVERLAP 0.13 0.15 0.17"
        command_iface.Clean()
        command_iface.LOQ()
        user_file = reduction_steps.UserFile()

        # Act
        user_file.read_line(line=line, reducer=ReductionSingleton())
        merge_Range = ReductionSingleton().instrument.getDetector("FRONT").mergeRange

        # Assert
        self.assertEqual(merge_Range.q_min, None, "The q_min should have been read in")
        self.assertEqual(merge_Range.q_max, None, "The q_max should have been read in")
        self.assertEqual(merge_Range.q_merge_range, False, "q_merge_range should be true")


class MockConvertTOQISISQResolution(object):
    def __init__(self):
        super(MockConvertTOQISISQResolution, self).__init__()

        self.delta_r = None
        self.a1 = None
        self.a2 = None
        self.w1 = None
        self.h1 = None
        self.w2 = None
        self.h2 = None
        self.collim = None
        self.on_off = None

    def set_q_resolution_delta_r(self, delta_r):
        self.delta_r = delta_r

    def set_q_resolution_a1(self, a1):
        self.a1 = a1

    def set_q_resolution_a2(self, a2):
        self.a2 = a2

    def set_q_resolution_collimation_length(self, collimation_length):
        self.collim = collimation_length

    def set_q_resolution_h1(self, h1):
        self.h1 = h1

    def set_q_resolution_h2(self, h2):
        self.h2 = h2

    def set_q_resolution_w1(self, w1):
        self.w1 = w1

    def set_q_resolution_w2(self, w2):
        self.w2 = w2

    def set_use_q_resolution(self, enabled):
        self.on_off = enabled


class MockReducerQResolution(object):
    def __init__(self):
        super(MockReducerQResolution, self).__init__()
        self.to_Q = MockConvertTOQISISQResolution()


class TestQResolutionInUserFile(unittest.TestCase):
    def test_that_good_input_is_accepted(self):
        # Arrange
        reducer = MockReducerQResolution()
        user_file = reduction_steps.UserFile()

        a1_val = 1
        a2_val = 2
        h1_val = 3
        w1_val = 4
        h2_val = 5
        w2_val = 6
        lcollim_val = 7
        delta_r_val = 8
        on_off_val = True
        values = [
            "QRESOL/A1=" + str(a1_val),
            "QRESOL/A2  =" + str(a2_val),
            "QRESOL/H1  =" + str(h1_val),
            "QRESOL/W1 =" + str(w1_val),
            "QRESOL/H2=" + str(h2_val),
            "QRESOL/W2=  " + str(w2_val),
            "QRESOL/LCOLLIM=  " + str(lcollim_val),
            "QRESOL/LCOLLIM=  " + str(lcollim_val),
            "QRESOL/DELTAR=  " + str(delta_r_val),
            "QRESOL/ ON",
        ]

        # Act
        for value in values:
            user_file.read_line(value, reducer)

        # Assert
        self.assertEqual(reducer.to_Q.a1, a1_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.a2, a2_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.h1, h1_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.h2, h2_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.w1, w1_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.w2, w2_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.delta_r, delta_r_val / 1000.0, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.collim, lcollim_val, "Should be the input in meters")
        self.assertEqual(reducer.to_Q.on_off, on_off_val, "Should be set to True")

    def test_that_non_float_type_for_a1_causes_error_warning(self):
        # Arrange
        reducer = MockReducerQResolution()
        user_file = reduction_steps.UserFile()
        a1_val = "sdf"
        value = "A1=" + str(a1_val)
        # Act
        error = user_file._read_q_resolution_line(value, reducer)
        # Assert
        self.assertNotEqual(error, None)


if __name__ == "__main__":
    unittest.main()
