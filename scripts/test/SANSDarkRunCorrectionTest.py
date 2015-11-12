import unittest
import mantid
from mantid.simpleapi import *
from mantid.kernel import DateAndTime
import DarkRunCorrection as dc

def create_sample_workspace_with_log(name, x_start, x_end, bin_width,
                                     log_name, start_time, number_of_times):
    CreateSampleWorkspace(OutputWorkspace = name,
                               NumBanks=1,
                               BankPixelWidth=1,
                               XMin=x_start,
                               XMax = x_end,
                               BinWidth = bin_width)
    ws = mtd[name]
    for i in range(0, number_of_times):
        val = 2
        date = DateAndTime(start_time)
        date +=  int(i*1e9) # Add nanoseconds
        AddTimeSeriesLog(ws, Name=log_name, Time=date.__str__().strip(), Value=val)


class DarkRunCorrectionTest(unittest.TestCase):

    def _do_test_dark_run_correction(self, use_mean, use_uniform, use_time, log_name):
         # Arrange
        x_start = 0
        x_end = 10
        bin_width = 1

        start_time_scatter = 0 # in nanoseconds
        number_of_times_scatter = 5 # in seconds

        start_time_dark_run = 7 # in nanoseconds
        number_of_times_dark_run = 13 # in seconds

        ws_scatter_name = "ws_scatter"
        create_sample_workspace_with_log(ws_scatter_name, x_start, x_end, bin_width,
                                                      log_name, start_time_scatter, number_of_times_scatter)
        ws_dark_name = "ws_dark"
        create_sample_workspace_with_log(ws_dark_name, x_start, x_end, bin_width,
                                            log_name, start_time_dark_run, number_of_times_dark_run)
        # Arrange
        correction = dc.DarkRunCorrection()
        correction.set_use_mean(use_mean)
        correction.set_use_uniform(use_uniform)
        correction.set_use_time(use_time)

        # Act 
        out_ws = correction.execute(mtd[ws_scatter_name], mtd[ws_dark_name])

        # Assert only that it runs and produces an output workspace
        # The algorithm correctness has already been tested in SANSDarkRunBackgroundCorrectionTest
        self.assertTrue(out_ws is not None)

        # Clean up
        mtd.remove(ws_scatter_name)
        mtd.remove(ws_dark_name)

    def test__mean__uniform__time__dark_run_correction_runs(self):
        use_mean = True
        use_uniform = True
        use_time = True 
        log_name = "good_frames"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__mean__uniform__no_time__dark_run_correction_runs(self):
        use_mean = True
        use_uniform = True
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__mean__no_uniform__time__dark_run_correction_runs(self):
        use_mean = True
        use_uniform = False
        use_time = True 
        log_name = "good_frames"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__mean__no_uniform__no_time__dark_run_correction_runs(self):
        use_mean = True
        use_uniform = False
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__no_mean__uniform__time__dark_run_correction_runs(self):
        use_mean = False
        use_uniform = True
        use_time = True 
        log_name = "good_frames"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__no_mean__uniform__no_time__dark_run_correction_runs(self):
        use_mean = False
        use_uniform = True
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__no_mean__no_uniform__time__dark_run_correction_runs(self):
        use_mean = False
        use_uniform = False
        use_time = True 
        log_name = "good_frames"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

    def test__no_mean__no_uniform__no_time__dark_run_correction_runs(self):
        use_mean = False
        use_uniform = False
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(use_mean, use_uniform, use_time, log_name)

class DarkRunNormalizationExtractorTest(unittest.TestCase):

    def _do_test_extraction(self, log_name, use_time):
         # Arrange
        x_start = 0
        x_end = 10
        bin_width = 1

        start_time_scatter = 0 # in nanoseconds
        number_of_times_scatter = 5 # in seconds
        end_time_scatter = start_time_scatter + (number_of_times_scatter-1)*1e9
        duration_scatter_nano_seconds = end_time_scatter - start_time_scatter

        start_time_dark_run = 7 # in nanoseconds
        number_of_times_dark_run = 13 # in seconds
        end_time_dark_run = start_time_dark_run + (number_of_times_dark_run-1)*1e9
        duration_dark_run_nano_seconds = end_time_dark_run - start_time_dark_run

        ws_scatter_name = "ws_scatter"
        create_sample_workspace_with_log(ws_scatter_name, x_start, x_end, bin_width,
                                                      log_name, start_time_scatter, number_of_times_scatter)
        ws_dark_name = "ws_dark"
        create_sample_workspace_with_log(ws_dark_name, x_start, x_end, bin_width,
                                            log_name, start_time_dark_run, number_of_times_dark_run)
        ws_scatter = mtd[ws_scatter_name]
        ws_dark = mtd[ws_dark_name]
        extractor = dc.DarkRunNormalizationExtractor()

        # Act
        ratio = extractor.extract_normalization(ws_scatter, ws_dark, use_time)

        # Assert
        expected_ratio = float(duration_scatter_nano_seconds)/float(duration_dark_run_nano_seconds )
        self.assertEqual(ratio, expected_ratio, "Should have the same ratio")

        # Clean up
        mtd.remove(ws_scatter_name)
        mtd.remove(ws_dark_name)

    def test_good_frames_logs_can_be_extracted(self):
        log_name = "good_frames"
        use_time = True

        self._do_test_extraction(log_name, use_time)

    def test_uamph_logs_can_be_extracted(self):
        log_name = "good_uah_log"
        use_time = False

        self._do_test_extraction(log_name, use_time)

    def test_that_throws_runtime_exception_for_missing_log(self):
        # Arrange
        x_start = 0
        x_end = 10
        bin_width = 1
        log_name = "WRONG LOG NAME"

        start_time_scatter = 0 # in nanoseconds
        number_of_times_scatter = 5 # in seconds

        start_time_dark_run = 7 # in nanoseconds
        number_of_times_dark_run = 13 # in seconds

        ws_scatter_name = "ws_scatter"
        create_sample_workspace_with_log(ws_scatter_name, x_start, x_end, bin_width,
                                                      log_name, start_time_scatter, number_of_times_scatter)
        ws_dark_name = "ws_dark"
        create_sample_workspace_with_log(ws_dark_name, x_start, x_end, bin_width,
                                            log_name, start_time_dark_run, number_of_times_dark_run)

        extractor = dc.DarkRunNormalizationExtractor()
        use_time = True

        # Act + Assert
        args= [mtd[ws_scatter_name], mtd[ws_dark_name], use_time]
        self.assertRaises(RuntimeError, extractor.extract_normalization,*args)

        # Clean up
        mtd.remove(ws_scatter_name)
        mtd.remove(ws_dark_name)

if __name__ == "__main__":
    unittest.main()
