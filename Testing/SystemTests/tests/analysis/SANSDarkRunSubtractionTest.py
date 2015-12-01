import unittest
import stresstesting
import mantid
from mantid.simpleapi import *
from mantid.kernel import DateAndTime
from isis_reduction_steps import DarkRunSubtraction
from SANSUserFileParser import DarkRunSettings
from SANSUtility import getFileAndName


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

    def test_that_subtracts_with_correct_single_dark_run_for_detector(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None
        run_number = self._get_dark_file()

        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings)

        expected_num_spectr_ws = 245768 - 9 + 1 # Total number of spectra in the original workspace from 9 to 245798
        self.assertTrue(scatter_workspace.getNumberHistograms() == expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        for i in range(0, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Detector entries should all be 0")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0,3]:
            self.assertFalse(all_entries_zero(monitor_workspace, i), "Monitor entries should not all be 0")

    def test_that_subtracts_with_correct_single_dark_run_and_multiple_settings(self):
        # Arrange
        use_time_1 = False
        use_mean_1 = False
        use_mon_1 = False
        mon_number_1 = None
        run_number = self._get_dark_file()

        use_time_2 = False
        use_mean_2 = False
        use_mon_2 = True
        mon_number_2 = [1]


        settings = []
        setting1 = self._get_dark_run_settings_object(run_number, use_time_1, use_mean_1,
                                                      use_mon_1, mon_number_1)
        setting2 = self._get_dark_run_settings_object(run_number, use_time_2, use_mean_2,
                                                      use_mon_2, mon_number_2)
        settings.append(setting1)
        settings.append(setting2)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings)

        expected_num_spectr_ws = 245768 - 9 + 1 # Total number of spectra in the original workspace from 9 to 245798
        self.assertTrue(scatter_workspace.getNumberHistograms() == expected_num_spectr_ws, "Should have 8 spectra")

        # Expect all entries to be 0 except for monitor 0. We selected monitor 1 and all detectors
        # for the subtraction. Hence only moniotr 0 would have been spared.
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        for i in range(0, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Detector entries should all be 0")

        for i in [0,2,3]:
            self.assertFalse(all_entries_zero(monitor_workspace, i), "Monitor entries should not all be 0")

        for i in mon_number_2:
             self.assertTrue(all_entries_zero(monitor_workspace, i), "Entries should all be 0")
    
    def test_that_subtracts_correct_added_file_type(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None

        # Create added workspace and have it saved out
        import SANSadd2
        SANSadd2.add_runs(('28827','28797'),'SANS2DTUBES', '.nxs',
                          rawTypes=('.add','.raw','.s*'), lowMem=False,
                          saveAsEvent=True, isOverlay = False)
        run_number = r'SANS2D00028797-add.nxs'
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, run_number)

        expected_num_spectr_ws = 245768 - 9 + 1 # Total number of spectra in the original workspace from 9 to 245798
        self.assertTrue(scatter_workspace.getNumberHistograms() == expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        for i in range(0, scatter_workspace.getNumberHistograms()):
             self.assertTrue(all_entries_zero(scatter_workspace, i), "Detector entries should all be 0")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0,3]:
            self.assertFalse(all_entries_zero(monitor_workspace, i), "Monitor entries should not all be 0")

        os.remove(os.path.join(config['defaultsave.directory'],run_number))
    
    def test_that_subtracts_correct_added_file_type_when_only_monitor_subtracted(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = True
        mon_number = [1]

        # Create added workspace and have it saved out
        import SANSadd2
        SANSadd2.add_runs(('28827','28797'),'SANS2DTUBES', '.nxs',
                          rawTypes=('.add','.raw','.s*'), lowMem=False,
                          saveAsEvent=True, isOverlay = False)
        run_number = r'SANS2D00028797-add.nxs'
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, run_number)

        expected_num_spectr_ws = 245768 - 9 + 1 # Total number of spectra in the original workspace from 9 to 245798
        self.assertTrue(scatter_workspace.getNumberHistograms() == expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP
        all_entries_zero = lambda ws, index : all([0.0 == element for element in ws.dataY(index)])

        # Some spectra might be zero, so we have to check that there is something which is not zero
        all_detectors_zero = True
        for i in range(0, scatter_workspace.getNumberHistograms()):
             all_detectors_zero = all_detectors_zero & all_entries_zero(scatter_workspace, i)
        self.assertFalse(all_detectors_zero, "There should be some detectors which are not zero")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0,2,3]:
            self.assertFalse(all_entries_zero(monitor_workspace, i), "Monitor0, Monitor2, Monitor3 entries should not all be 0")

        # Monitor 1 should be 0
        for i in mon_number:
            self.assertTrue(all_entries_zero(monitor_workspace, i), "Monitor1 entries should  all be 0")

        os.remove(os.path.join(config['defaultsave.directory'],run_number))

    #------- HELPER Methods
    def _do_test_valid(self, settings, run_number = None):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        for setting in settings:
            dark_run_subtractor.add_setting(setting)

        # Create an actual scatter workspace
        scatter_workspace, monitor_workspace = self._get_sample_workspaces(run_number)

        # Execute the dark_run_subtractor
        try:
            start_spec = 9
            end_spec = 245768 # Full specturm length
            scatter_workspace, monitor_workspace = dark_run_subtractor.execute(scatter_workspace, monitor_workspace, start_spec, end_spec)
        except:
            self.assertFalse(True, "The DarkRunSubtraction executed with an error")

        return scatter_workspace, monitor_workspace

    def _get_dark_file(self):
        # Provide an event file from the system test data repo
        return "SANS2D00028827.nxs"

    def _get_sample_workspaces(self, run_number = None):
        ws = None
        monitor_ws = None
        file_path = None
        ws_name = None
        sample_ws = None
        if run_number is not None:
            file_path, ws_name= getFileAndName(run_number)
            alg_load = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load.initialize()
            alg_load.setChild(True)
            alg_load.setProperty("Filename", file_path)
            alg_load.setProperty("EntryNumber", 1)
            alg_load.setProperty("OutputWorkspace", ws_name)
            alg_load.execute()
            sample_ws = alg_load.getProperty("OutputWorkspace").value
        else:
            file_path, ws_name= getFileAndName(self._get_dark_file())
            alg_load = AlgorithmManager.createUnmanaged("Load")
            alg_load.initialize()
            alg_load.setChild(True)
            alg_load.setProperty("Filename", file_path)
            alg_load.setProperty("OutputWorkspace", ws_name)
            alg_load.execute()
            sample_ws = alg_load.getProperty("OutputWorkspace").value

        # Now get the monitor
        monitor_ws = None
        if run_number is not None:
            ws_name2 = "monitors"
            alg_load2 = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load2.initialize()
            alg_load2.setChild(True)
            alg_load2.setProperty("Filename", file_path)
            alg_load2.setProperty("EntryNumber", 2)
            alg_load2.setProperty("OutputWorkspace", ws_name2)
            alg_load2.execute()
            monitor_ws = alg_load2.getProperty("OutputWorkspace").value
        else:
            # Load the monitor workspace
            monitors_name = ws_name + "_monitors"
            alg_load_monitors = AlgorithmManager.createUnmanaged("LoadNexusMonitors")
            alg_load_monitors.initialize()
            alg_load_monitors.setChild(True)
            alg_load_monitors.setProperty("Filename", file_path)
            alg_load_monitors.setProperty("MonitorsAsEvents", False)
            alg_load_monitors.setProperty("OutputWorkspace", monitors_name)
            alg_load_monitors.execute()
            monitor_ws = alg_load_monitors.getProperty("OutputWorkspace").value

        # Rebin the scatter data to match the monitor binning
        rebinned_name = "monitor_rebinned"
        alg_rebin = AlgorithmManager.createUnmanaged("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", sample_ws)
        alg_rebin.setProperty("WorkspaceToMatch", monitor_ws)
        alg_rebin.setProperty("PreserveEvents", False)
        alg_rebin.setProperty("OutputWorkspace", rebinned_name)
        alg_rebin.execute()
        sample_ws = alg_rebin.getProperty("OutputWorkspace").value

        return sample_ws, monitor_ws

    def _get_dark_run_settings_object(self, run, time, mean,
                                      use_mon, mon_number):
        # This is what would be coming from a parsed user file
        return DarkRunSettings(mon = use_mon,
                               run_number = run,
                               time = time,
                               mean = mean,
                               mon_number = mon_number)


class DarkRunSubtractionTestStressTest(stresstesting.MantidStressTest):
    def runTest(self):
        self._success = False
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(DarkRunSubtractionTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success


if __name__ == '__main__':
    unittest.main()