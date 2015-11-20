import unittest
import mantid
from mantid.simpleapi import *
from mantid.kernel import DateAndTime
from isis_reduction_steps import DarkRunSubtraction
from SANSUserFileParser import DarkRunSettings
from SANSDarkRunCorrectionTest import create_real_workspace_with_log

class DarkRunSubtractionTest(unittest.TestCase):
    def test_that_specifying_more_than_two_run_numbers_per_category_raises_error(self):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        # Time-based detectors
        setting1 = self._get_dark_run_settings_object("111111", True, False, False, None)
        setting2 = self._get_dark_run_settings_object("222222", True, False, False, None)
        # Uamp-based detectors
        setting3 = self._get_dark_run_settings_object("111111", False, False, False, None)
        setting4 = self._get_dark_run_settings_object("222222", False, False, False, None)
        # Time-based monitors
        setting5 = self._get_dark_run_settings_object("111111", True, False, True, None)
        setting6 = self._get_dark_run_settings_object("222222", True, False, True, None)
        # Uamp-based monitors
        setting7 = self._get_dark_run_settings_object("111111", False, False, True, None)
        setting8 = self._get_dark_run_settings_object("222222", False, False, True, None)

        dark_run_subtractor.add_setting(setting1)
        dark_run_subtractor.add_setting(setting2)
        dark_run_subtractor.add_setting(setting3)
        dark_run_subtractor.add_setting(setting4)
        dark_run_subtractor.add_setting(setting5)
        dark_run_subtractor.add_setting(setting6)
        dark_run_subtractor.add_setting(setting7)
        dark_run_subtractor.add_setting(setting8)
        # Act + Assert
        self.assertRaises(RuntimeError, dark_run_subtractor.get_time_based_setting_detectors)
        self.assertRaises(RuntimeError, dark_run_subtractor.get_uamp_based_setting_detectors)
        self.assertRaises(RuntimeError, dark_run_subtractor.get_time_based_setting_monitors)
        self.assertRaises(RuntimeError, dark_run_subtractor.get_uamp_based_setting_monitors)

    def test_that_raises_when_detector_has_more_than_one_setting(self):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        # We have two settings with different run numbers for detecetor-type corrections
        setting1 = self._get_dark_run_settings_object("222222", True, False, False, None)
        setting2 = self._get_dark_run_settings_object("222222", True, False, False, None)
        dark_run_subtractor.add_setting(setting1)
        dark_run_subtractor.add_setting(setting2)
        # Act + Assert
        self.assertRaises(RuntimeError, dark_run_subtractor.get_time_based_setting_detectors)

    def test_that_raises_when_having_mixed_mean_settings_for_monitor_time(self):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        # When having two monitor settings with differing mean selections, this is inconsistent
        setting1 = self._get_dark_run_settings_object("222222", True, True, True, None)
        setting2 = self._get_dark_run_settings_object("222222", True, False, True, None)
        dark_run_subtractor.add_setting(setting1)
        dark_run_subtractor.add_setting(setting2)
        # Act + Assert
        self.assertRaises(RuntimeError, dark_run_subtractor.get_time_based_setting_detectors)

    def test_that_subtracts_with_correct_single_dark_run(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None
        run_number_name = "test_run_number1111"
        run_number = self._create_dark_file(run_number_name)
        saved_files = []
        saved_files.append(run_number)

        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        scatter_workspace = self._do_test_valid(settings, saved_files, ["good_frames", "good_uah_log"])
        self.assertTrue(scatter_workspace.getNumberHistograms() == 8, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0 and the monitors(index 0 and 1) are not. This is because we
        # subtract bin by bin when using UAMP
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        for i in [0,1]:
            self.assertFalse(all_entries_zero(scatter_workspace, i), "Monitor entries should not all be 0")

        for i in range(2, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Detector entries should all be 0")

    def test_that_subtracts_with_correct_single_dark_run_and_multiple_settings(self):
        # Arrange
        use_time_1 = False
        use_mean_1 = False
        use_mon_1 = False
        mon_number_1 = None
        run_number_name = "test_run_number1111"

        run_number = self._create_dark_file(run_number_name)
        saved_files = []
        saved_files.append(run_number)

        use_time_2 = False
        use_mean_2 = False
        use_mon_2 = True
        mon_number_2 = [1]
        saved_files = []

        settings = []
        setting1 = self._get_dark_run_settings_object(run_number, use_time_1, use_mean_1,
                                                      use_mon_1, mon_number_1)
        setting2 = self._get_dark_run_settings_object(run_number, use_time_2, use_mean_2,
                                                      use_mon_2, mon_number_2)
        settings.append(setting1)
        settings.append(setting2)

        # Act + Assert
        scatter_workspace = self._do_test_valid(settings, saved_files, ["good_uah_log", "good_frames"])
        self.assertTrue(scatter_workspace.getNumberHistograms() == 8, "Should have 8 spectra")

        # Expect all entries to be 0 except for monitor 0. We selected monitor 1 and all detectors
        # for the subtraction. Hence only moniotr 0 would have been spared.
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        for i in [0]:
            self.assertFalse(all_entries_zero(scatter_workspace, i), "Monitor entries should not all be 0")

        for i in range(1, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Entries should all be 0")

    def test_that_subtracts_with_two_correct_dark_runs(self):
        # Arrange
        use_time_1 = False
        use_mean_1 = False
        use_mon_1 = False
        mon_number_1 = None
        run_number_1_name = "test_run_number1111"

        run_number_1 = self._create_dark_file(run_number_1_name)
        saved_files = []
        saved_files.append(run_number_1)

        use_time_2 = True
        use_mean_2 = False
        use_mon_2 = True
        mon_number_2 = [1]
        run_number_2_name = "test_run_number2222"

        run_number_2 = self._create_dark_file(run_number_2_name)
        saved_files = []
        saved_files.append(run_number_2)

        settings = []
        setting1 = self._get_dark_run_settings_object(run_number_1, use_time_1, use_mean_1,
                                                      use_mon_1, mon_number_1)
        setting2 = self._get_dark_run_settings_object(run_number_2, use_time_2, use_mean_2,
                                                      use_mon_2, mon_number_2)
        settings.append(setting1)
        settings.append(setting2)

        # Act + Assert
        scatter_workspace = self._do_test_valid(settings, saved_files, ["good_uah_log", "good_frames"])
        self.assertTrue(scatter_workspace.getNumberHistograms() == 8, "Should have 8 spectra")

        # Expect all entries to be 0 except for monitor 0. We selected monitor 1 and all detectors
        # for the subtraction. Hence only moniotr 0 would have been spared. The detecors should be 0, because
        # we used UAMP on them and monitor1 should have changed its values
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        # Monitor 0
        for i in [0]:
            self.assertFalse(all_entries_zero(scatter_workspace, i), "Monitor entries should not all be 0")

        # Detectors
        for i in range(2, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Entries should all be 0")

        # Monitor 1
        ref_ws = create_real_workspace_with_log("ref_ws_test", ["good_uah_log","good_frames"] , 0, 10)
        some_entries_different = lambda ref_ws, ws, index : any([el1 != el2 for el1, el2 in
                                                                zip(ref_ws.dataY(index), ws.dataY(index))])
        self.assertTrue(some_entries_different(ref_ws, scatter_workspace, 1))

    #------- HELPER Methods
    def _do_test_valid(self, settings, saved_files, type):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        for setting in settings:
            dark_run_subtractor.add_setting(setting)

        # Create an actual scatter workspace
        scatter_workspace = create_real_workspace_with_log("scatter", type, 0, 10)
        # Execute the dark_run_subtractor
        output = None
        try:
            output = dark_run_subtractor.execute(scatter_workspace)
        except:
            self._remove_files(saved_files)
            self.assertFalse(True, "The DarkRunSubtraction executed with an error")
        # We need to delete any test file which was created
        self._remove_files(saved_files)

        return output

    def _create_dark_file(self, run_number):
        # Create dark file
        dark_run_name = "dark_run" + run_number

        dark_run_ws = create_real_workspace_with_log("scatter", ["good_uah_log","good_frames"] , 0, 10)

        # Save in the temporary save location of mantid
        temp_save_dir = config['defaultsave.directory']
        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()

        file_name = os.path.join(temp_save_dir, run_number + '.nxs')

        alg_save  = AlgorithmManager.create("SaveNexusProcessed")
        alg_save.initialize()
        alg_save.setChild(True)
        alg_save.setProperty("InputWorkspace", dark_run_ws)
        alg_save.setProperty("Filename", file_name)
        alg_save.execute()
        return file_name

    def _remove_files(self, saved_files):
        for file_name in saved_files:
            if os.path.exists(file_name):
                os.remove(file_name)

    def _get_dark_run_settings_object(self, run, time, mean,
                                      use_mon, mon_number):
        # This is what would be coming from a parsed user file
        return DarkRunSettings(mon = use_mon,
                               run_number = run,
                               time = time,
                               mean = mean,
                               mon_number = mon_number)

if __name__ == "__main__":
    unittest.main()