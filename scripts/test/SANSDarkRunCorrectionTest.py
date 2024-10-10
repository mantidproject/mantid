# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmManager, mtd
from mantid.simpleapi import CreateSampleWorkspace
from mantid.kernel import DateAndTime
import DarkRunCorrection as dc


# --- Helper Functions
def add_log(ws, number_of_times, log_name, start_time):
    alg_log = AlgorithmManager.create("AddTimeSeriesLog")
    alg_log.initialize()
    alg_log.setChild(True)
    alg_log.setProperty("Workspace", ws)
    alg_log.setProperty("Name", log_name)

    # Add the data
    if log_name == "good_frames":
        convert_to_type = int
    else:
        convert_to_type = float

    for i in range(0, number_of_times):
        date = DateAndTime(start_time)
        date += int(i * 1e9)  # Add nanoseconds
        alg_log.setProperty("Time", date.__str__().strip())
        alg_log.setProperty("Value", convert_to_type(i))
        alg_log.execute()

    return alg_log.getProperty("Workspace").value


def create_sample_workspace_with_log(name, x_start, x_end, bin_width, log_name, start_time, number_of_times):
    CreateSampleWorkspace(OutputWorkspace=name, NumBanks=1, BankPixelWidth=1, XMin=x_start, XMax=x_end, BinWidth=bin_width)
    ws = mtd[name]
    ws = add_log(ws, number_of_times, log_name, start_time)

    # We need the gd_prtn_chrg if "good_uah_log" present
    if "good_uah_log" in log_name:
        alg = AlgorithmManager.create("AddSampleLog")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("Workspace", ws)
        alg.setProperty("LogName", "gd_prtn_chrg")
        alg.setProperty("LogText", str(ws.getRun().getProperty("good_uah_log").value[-1]))
        alg.setProperty("LogType", "Number")
        alg.execute()


def create_real_workspace_with_log(name, log_names, start_time, number_of_times):
    filename = "LOQ48127np.nxs"
    out_ws_name = name
    alg_load = AlgorithmManager.create("LoadNexusProcessed")
    alg_load.initialize()
    alg_load.setChild(True)
    alg_load.setProperty("Filename", filename)
    alg_load.setProperty("OutputWorkspace", out_ws_name)
    alg_load.execute()
    ws = alg_load.getProperty("OutputWorkspace").value

    for log_name in log_names:
        ws = add_log(ws, number_of_times, log_name, start_time)

    # We need the gd_prtn_chrg if "good_uah_log" present
    if "good_uah_log" in log_names:
        alg = AlgorithmManager.create("AddSampleLog")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("Workspace", ws)
        alg.setProperty("LogName", "gd_prtn_chrg")
        alg.setProperty("LogText", str(ws.getRun().getProperty("good_uah_log").value[-1]))
        alg.setProperty("LogType", "Number")
        alg.execute()
    return ws


def create_real_event_workspace_with_log(name, log_names, start_time, number_of_times):
    filename = "CNCS_7860_event.nxs"
    out_ws_name = name
    alg_load = AlgorithmManager.create("LoadNexusProcessed")
    alg_load.initialize()
    alg_load.setChild(True)
    alg_load.setProperty("Filename", filename)
    alg_load.setProperty("OutputWorkspace", out_ws_name)
    alg_load.execute()
    ws = alg_load.getProperty("OutputWorkspace").value

    for log_name in log_names:
        ws = add_log(ws, number_of_times, log_name, start_time)
    return ws


# ----- TESTS
class DarkRunCorrectionTest(unittest.TestCase):
    def _do_test_dark_run_correction(
        self, log_name, use_mean, use_time, use_detectors=True, use_monitors=False, mon_number=None, use_real_data=False
    ):
        # Arrange
        if mon_number is None:
            mon_number = []

        x_start = 0
        x_end = 10
        bin_width = 1

        start_time_scatter = 0  # in nanoseconds
        number_of_times_scatter = 5  # in seconds

        start_time_dark_run = 7  # in nanoseconds
        number_of_times_dark_run = 13  # in seconds

        ws_scatter_name = "ws_scatter"
        ws_dark_name = "ws_dark"

        ws_scatter = None
        ws_dark = None

        # We either load some data or create sample data
        if use_real_data:
            ws_scatter = create_real_workspace_with_log("scatter", [log_name], start_time_scatter, number_of_times_scatter)
            ws_dark = create_real_workspace_with_log("dark", [log_name], start_time_dark_run, number_of_times_dark_run)
        else:
            create_sample_workspace_with_log(
                ws_scatter_name, x_start, x_end, bin_width, log_name, start_time_scatter, number_of_times_scatter
            )

            create_sample_workspace_with_log(
                ws_dark_name, x_start, x_end, bin_width, log_name, start_time_dark_run, number_of_times_dark_run
            )
            ws_scatter = mtd[ws_scatter_name]
            ws_dark = mtd[ws_dark_name]

        correction = dc.DarkRunCorrection()
        correction.set_use_mean(use_mean)
        correction.set_use_time(use_time)
        correction.set_use_detectors(use_detectors)
        correction.set_use_monitors(use_monitors)
        correction.set_mon_numbers(mon_number)

        # Act
        out_ws = correction.execute(ws_scatter, ws_dark)

        # Assert only that it runs and produces an output workspace
        # The algorithm correctness has already been tested in SANSDarkRunBackgroundCorrectionTest
        self.assertNotEqual(out_ws, None)

        # Clean up
        if not use_real_data:
            mtd.remove(ws_scatter_name)
            mtd.remove(ws_dark_name)

    def test__mean__time__dark_run_correction_runs(self):
        use_mean = True
        use_time = True
        log_name = "good_frames"
        self._do_test_dark_run_correction(log_name, use_mean, use_time)

    def test__mean__no_time__dark_run_correction_runs(self):
        use_mean = True
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(log_name, use_mean, use_time)

    def test__no_mean__time__dark_run_correction_runs(self):
        use_mean = False
        use_time = True
        log_name = "good_frames"
        self._do_test_dark_run_correction(log_name, use_mean, use_time)

    def test__no_mean__no_time__dark_run_correction_runs(self):
        use_mean = False
        use_time = False
        log_name = "good_uah_log"
        self._do_test_dark_run_correction(log_name, use_mean, use_time)

    ## -- The tests below make sure that monitor and detector settings can be applied
    def test__mean__time__all_monitors__no_detectors(self):
        use_mean = True
        use_time = True
        log_name = "good_frames"
        use_detectors = False
        use_monitors = True
        mon_numbers = []

        self._do_test_dark_run_correction(log_name, use_mean, use_time, use_detectors, use_monitors, mon_numbers, use_real_data=True)

    def test__mean__no_time__one_monitor__no_detectors(self):
        use_mean = True
        use_time = False
        log_name = "good_uah_log"
        use_detectors = False
        use_monitors = True
        mon_numbers = [1]

        self._do_test_dark_run_correction(log_name, use_mean, use_time, use_detectors, use_monitors, mon_numbers, use_real_data=True)

    def test__no_mean__time__one_monitor__detectors(self):
        use_mean = False
        use_time = True
        log_name = "good_frames"
        use_detectors = True
        use_monitors = True
        mon_numbers = [1]

        self._do_test_dark_run_correction(log_name, use_mean, use_time, use_detectors, use_monitors, mon_numbers, use_real_data=True)

    def test_no_mean__no_time__all_monitors__detectors(self):
        use_mean = False
        use_time = False
        log_name = "good_uah_log"
        use_detectors = True
        use_monitors = True
        mon_numbers = []

        self._do_test_dark_run_correction(log_name, use_mean, use_time, use_detectors, use_monitors, mon_numbers, use_real_data=True)


class DarkRunNormalizationExtractorTest(unittest.TestCase):
    def _do_test_extraction(self, log_name, use_time):
        # Arrange
        x_start = 0
        x_end = 10
        bin_width = 1

        start_time_scatter = 0  # in nanoseconds
        number_of_times_scatter = 5  # in seconds

        start_time_dark_run = 7  # in nanoseconds
        number_of_times_dark_run = 13  # in seconds

        ws_scatter_name = "ws_scatter"
        create_sample_workspace_with_log(ws_scatter_name, x_start, x_end, bin_width, log_name, start_time_scatter, number_of_times_scatter)
        ws_dark_name = "ws_dark"
        create_sample_workspace_with_log(ws_dark_name, x_start, x_end, bin_width, log_name, start_time_dark_run, number_of_times_dark_run)
        ws_scatter = mtd[ws_scatter_name]
        ws_dark = mtd[ws_dark_name]

        extractor = dc.DarkRunNormalizationExtractor()

        # Act
        ratio = extractor.extract_normalization(ws_scatter, ws_dark, use_time)

        # Assert
        if log_name == "good_frames":
            time_frame_scatter = ws_scatter.dataX(0)[-1] - ws_scatter.dataX(0)[0]
            number_of_frames_scatter = ws_scatter.getRun().getProperty("good_frames").value[-1]

            time_frame_dark = ws_dark.dataX(0)[-1] - ws_dark.dataX(0)[0]
            number_of_frames_dark = ws_dark.getRun().getProperty("good_frames").value[-1]

            expected_ratio = float(time_frame_scatter * number_of_frames_scatter) / float(time_frame_dark * number_of_frames_dark)
        else:
            expected_ratio = ws_scatter.getRun().getProperty("gd_prtn_chrg").value / ws_dark.getRun().getProperty("gd_prtn_chrg").value

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

        start_time_scatter = 0  # in nanoseconds
        number_of_times_scatter = 5  # in seconds

        start_time_dark_run = 7  # in nanoseconds
        number_of_times_dark_run = 13  # in seconds

        ws_scatter_name = "ws_scatter"
        create_sample_workspace_with_log(ws_scatter_name, x_start, x_end, bin_width, log_name, start_time_scatter, number_of_times_scatter)
        ws_dark_name = "ws_dark"
        create_sample_workspace_with_log(ws_dark_name, x_start, x_end, bin_width, log_name, start_time_dark_run, number_of_times_dark_run)

        extractor = dc.DarkRunNormalizationExtractor()
        use_time = True

        # Act + Assert
        args = [mtd[ws_scatter_name], mtd[ws_dark_name], use_time]
        self.assertRaises(RuntimeError, extractor.extract_normalization, *args)

        # Clean up
        mtd.remove(ws_scatter_name)
        mtd.remove(ws_dark_name)


if __name__ == "__main__":
    unittest.main()
