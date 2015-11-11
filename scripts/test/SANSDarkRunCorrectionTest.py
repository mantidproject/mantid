import unittest
import mantid
from mantid.simpleapi import *

import DarkRunCorrection as dc

def create_sample_workspace_with_log(x_start, x_end, bin_width,
                                     log_name, start_time, number_of_times):
    ws = CreateSampleWorkspace(NumBanks=1,
                               BankPixelWidth=1,
                               XMin=x_start,
                               XMax = x_end,
                               BinWidth = bin_width)
    for i in range(0, number_of_times):
        val = random.randrange(0, 10, 1)
        date = DateAndTime(start_time)
        date +=  int(i*1e9)
        AddTimeSeriesLog(ws, Name=log_name, Time=date.__str__().strip(), Value=val)
    return ws


class SANSDarkRunCorrectionTest(unittest.TestCase):
    def test_standard_dark_run_correction(self):
        self.assertTrue(False)

class DarkRunNormalizationExtractorTest(unittest.TestCase):
    def test_good_frames_logs_can_be_extracted(self):
        # Arrange
        x_start = 0
        bin_width = 1
        log_name = "good_frame"

        start_time_scatter = 3
        number_of_times_scatter = 20

        start_time_dark_run = 7
        number_of_times_dark_run = 45

        ws_scatter = create_sample_workspace_with_log(x_start, x_end, bin_width,
                                            log_name, start_time_scatter, number_of_times_scatter)
        ws_dark = create_sample_workspace_with_log(x_start, x_end, bin_width,
                                            log_name, start_time_dark_run, number_of_times_dark_run)

        extractor = dc.DarkRunNormalizationExtractor()
        use_time = True

        # Act
        ratio = extractor.extract_normalization(ws_scatter, dark_run, use_time)
        print "###########"
        print ratio
        # Assert
        expected_ratio = number_of_times_scatter/number_of_times_dark_run -1
        self.assertEqual(ratio, expected_ratio, "Should have the same ratio")

    def test_that_throws_runtime_exception_for_missing_log(self):
         # Arrange
        x_start = 0
        bin_width = 1
        log_name = "alsdkfjslkdf"

        start_time_scatter = 3
        number_of_times_scatter = 20

        start_time_dark_run = 7
        number_of_times_dark_run = 45

        ws_scatter = create_sample_workspace_with_log(x_start, x_end, bin_width,
                                            log_name, start_time_scatter, number_of_times_scatter)
        ws_dark = create_sample_workspace_with_log(x_start, x_end, bin_width,
                                            log_name, start_time_dark_run, number_of_times_dark_run)

        extractor = dc.DarkRunNormalizationExtractor()
        use_time = True

        # Act + Assert
        self.assertRaises(RuntimeError, extractor.extract_normalization, [ws_scatter, dark_run, use_time])



