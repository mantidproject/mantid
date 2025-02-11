# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
# pylint: disable=invalid-name
# pylint: disable=too-many-arguments
# pylint: disable=too-many-public-methods
import unittest
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.api import AlgorithmManager
from mantid.kernel import config
from isis_reduction_steps import DarkRunSubtraction
from SANSUserFileParser import DarkRunSettings
from SANSUtility import getFileAndName
import numpy as np
import os

from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
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

        is_input_event = True

        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, is_input_event)

        expected_num_spectr_ws = 20 - 9 + 1
        self.assertEqual(scatter_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 10 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP
        y = scatter_workspace.extractY()
        self.assertFalse(np.greater(y, 1e-14).any(), "Detector entries should all be 0")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        self.assertTrue(monitor_workspace.extractY()[:, 0:3].any(), "Monitor entries should not all be 0")

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
        mon_number_2 = [2]  # We are selecting detector ID 2 this corresponds to workspace index 1
        ws_index2 = [1]

        settings = []
        setting1 = self._get_dark_run_settings_object(run_number, use_time_1, use_mean_1, use_mon_1, mon_number_1)
        setting2 = self._get_dark_run_settings_object(run_number, use_time_2, use_mean_2, use_mon_2, mon_number_2)
        settings.append(setting1)
        settings.append(setting2)

        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings)

        expected_num_spectr_ws = 20 - 9 + 1  # Total number of spectra in the original workspace from 9 to 245798
        self.assertEqual(scatter_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 8 spectra")

        # Expect all entries to be 0 except for monitor 0. We selected monitor 1 and all detectors
        # for the subtraction. Hence only moniotr 0 would have been spared.

        y = scatter_workspace.extractY()
        self.assertFalse(np.greater(y, 1e-14).any(), "Detector entries should all be 0")

        for i in [0, 2, 3]:
            self.assertTrue(monitor_workspace.dataY(i).any(), "Monitor entries should not all be 0")

        for i in ws_index2:
            y = monitor_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Entries should all be 0")

    def test_that_subtracts_correct_added_file_type(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None

        # Create added workspace and have it saved out
        import SANSadd2

        SANSadd2.add_runs(
            ("SANS2D00028827_removed_spectra.nxs", "SANS2D00028797_removed_spectra.nxs"),
            "SANS2DTUBES",
            ".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=False,
        )
        run_number = r"SANS2D00028797_removed_spectra-add.nxs"
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        is_event_ws = True
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, is_event_ws, run_number)
        expected_num_spectr_ws = 20 - 9 + 1
        self.assertEqual(scatter_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP
        y = scatter_workspace.extractY()
        self.assertFalse(np.greater(y, 1e-14).any(), "Detector entries should all be 0")
        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0, 3]:
            self.assertTrue(monitor_workspace.dataY(i).any(), "Monitor entries should not all be 0")

        os.remove(os.path.join(config["defaultsave.directory"], run_number))

    def test_that_subtracts_correct_added_file_type_when_only_monitor_subtracted(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = True
        mon_number = [2]  # We are selecting detector ID 2 this corresponds to workspace index 1
        ws_index = [1]

        # Create added workspace and have it saved out
        import SANSadd2

        SANSadd2.add_runs(
            ("SANS2D00028827_removed_spectra.nxs", "SANS2D00028797_removed_spectra.nxs"),
            "SANS2DTUBES",
            ".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=False,
        )
        run_number = r"SANS2D00028797_removed_spectra-add.nxs"
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        # Act + Assert
        is_event_ws = True
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, is_event_ws, run_number)

        expected_num_spectr_ws = 20 - 9 + 1  # Total number of spectra in the original workspace from 9 to 245798
        self.assertEqual(scatter_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # Some spectra might be zero, so we have to check that there is something which is not zero
        self.assertTrue(scatter_workspace.extractY().any(), "There should be some detectors which are not zero")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0, 2, 3]:
            self.assertTrue(monitor_workspace.dataY(i).any(), "Monitor1, Monitor3, Monitor4 entries should not all be 0")

        # Monitor 2 (workspace index 1 should be 0
        for i in ws_index:
            y = monitor_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Monitor2 entries should  all be 0")

        os.remove(os.path.join(config["defaultsave.directory"], run_number))

    def test_that_subtracts_correct_for_histo_input_workspace(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = True
        mon_number = [2]
        ws_index = [1]

        run_number = self._get_dark_file()
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        is_event_input = False
        # Act + Assert
        scatter_workspace, monitor_workspace = self._do_test_valid(settings, is_event_input, run_number)

        expected_num_spectr_ws = 20 - 9 + 1
        self.assertEqual(scatter_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 8 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # Some spectra might be zero, so we have to check that there is something which is not zero
        self.assertTrue(scatter_workspace.extractY().any(), "There should be some detectors which are not zero")

        # The monitors should not be affected, but we only have data in ws_index 0-3
        for i in [0, 2, 3]:
            self.assertTrue(monitor_workspace.dataY(i).any(), "Monitor1, Monitor3, Monitor4 entries should not all be 0")

        # Monitor 1 should be 0
        for i in ws_index:
            y = monitor_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Monitor2 entries should  all be 0")

    def test_that_subtracts_correct_for_transmission_workspace_with_only_monitors(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = True
        mon_number = [2]
        ws_index = [1]

        run_number = self._get_dark_file()
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        trans_ids = [1, 2, 3, 4]

        # Act + Assert
        transmission_workspace = self._do_test_valid_transmission(settings, trans_ids)

        expected_num_spectr_ws = len(trans_ids)
        self.assertEqual(transmission_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 4 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # We only have monitors in our transmission file, monitor 2 should be 0
        for i in [0, 2, 3]:
            self.assertTrue(transmission_workspace.dataY(i).any(), "Monitor1, Monitor3, Monitor4 entries should not all be 0")

        # Monitor2 should be 0
        for i in ws_index:
            y = transmission_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Monitor2 entries should  all be 0")

    def test_that_subtracts_nothing_when_selecting_detector_subtraction_for_transmission_workspace_with_only_monitors(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None

        run_number = self._get_dark_file()
        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        trans_ids = [1, 2, 3, 4]

        # Act + Assert
        transmission_workspace = self._do_test_valid_transmission(settings, trans_ids)

        expected_num_spectr_ws = len(trans_ids)
        self.assertEqual(transmission_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have 4 spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # We only have monitors in our transmission file
        for i in [0, 1, 2, 3]:
            self.assertTrue(
                transmission_workspace.dataY(i).any(), ("Monitor1, Monitor2, Monitor3 and Monitor4 entries should not all be 0")
            )

    def test_that_subtracts_monitors_and_detectors_for_transmission_workspace_with_monitors_and_detectors(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = False
        mon_number = None
        run_number = self._get_dark_file()

        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        use_time2 = False
        use_mean2 = False
        use_mon2 = True
        mon_number2 = [2]
        ws_index2 = [1]
        run_number2 = self._get_dark_file()

        setting2 = self._get_dark_run_settings_object(run_number2, use_time2, use_mean2, use_mon2, mon_number2)
        settings.append(setting2)

        monitor_ids = [1, 2, 3, 4]
        trans_ids = monitor_ids
        detector_ids = list(range(1100000, 1100010))
        trans_ids.extend(detector_ids)

        # Act + Assert
        transmission_workspace = self._do_test_valid_transmission(settings, trans_ids)

        expected_num_spectr_ws = len(trans_ids)  # monitors + detectors
        self.assertEqual(transmission_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have the same number of spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # We only have monitors in our transmission file, monitor 1 should be 0
        for i in [0, 2, 3]:
            self.assertTrue(transmission_workspace.dataY(i).any(), "Monitor0, Monitor2, Monitor3 entries should not all be 0")

        # Monitor 2 should be set to 0
        for i in ws_index2:
            y = transmission_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Monitor2 entries should be 0")

        # Detectors should be set to 0
        detector_indices = list(range(4, 14))
        for i in detector_indices:
            y = transmission_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "All detectors entries should be 0")

    def test_that_subtracts_monitors_only_for_transmission_workspace_with_monitors_and_detectors(self):
        # Arrange
        use_time = False
        use_mean = False
        use_mon = True
        mon_number = [2]
        ws_index = [1]
        run_number = self._get_dark_file()

        settings = []
        setting = self._get_dark_run_settings_object(run_number, use_time, use_mean, use_mon, mon_number)
        settings.append(setting)

        monitor_ids = [1, 2, 3, 4]
        trans_ids = monitor_ids
        detector_ids = list(range(1100000, 1100010))
        trans_ids.extend(detector_ids)

        # Act + Assert
        transmission_workspace = self._do_test_valid_transmission(settings, trans_ids)

        expected_num_spectr_ws = len(trans_ids)  # monitors + detectors
        self.assertEqual(transmission_workspace.getNumberHistograms(), expected_num_spectr_ws, "Should have all spectra")

        # Since in this test we use the same file for the scatterer and the dark run, we expect
        # that the detectors are 0. This is because we subtract bin by bin when using UAMP

        # We only have monitors in our transmission file, monitor 1 should be 0
        for i in [0, 2, 3]:
            self.assertTrue(transmission_workspace.dataY(i).any(), "Monitor0, Monitor2, Monitor3 entries should not all be 0")

        # Monitor 2 should be set to 0
        for i in ws_index:
            y = transmission_workspace.dataY(i)
            self.assertFalse(np.greater(y, 1e-14).any(), "Monitor2 entries should be 0")

        # Detectors should NOT all be set to 0
        self.assertTrue(transmission_workspace.extractY().any(), "There should be some detectors which are not zero")

    # ------- HELPER Methods
    def _do_test_valid(self, settings, is_input_event=True, run_number=None):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        for setting in settings:
            dark_run_subtractor.add_setting(setting)

        # Create an actual scatter workspace
        scatter_workspace = None
        monitor_workspace = None
        if is_input_event:
            scatter_workspace, monitor_workspace = self._get_sample_workspaces(run_number)
        else:
            scatter_workspace, monitor_workspace = self._get_sample_workspace_histo()

        # Execute the dark_run_subtractor - let exceptions flow to help track down errors
        start_spec = 9
        end_spec = 20  # Full spectrum length
        scatter_workspace, monitor_workspace = dark_run_subtractor.execute(
            scatter_workspace, monitor_workspace, start_spec, end_spec, is_input_event
        )

        return scatter_workspace, monitor_workspace

    def _do_test_valid_transmission(self, settings, trans_ids):
        # Arrange
        dark_run_subtractor = DarkRunSubtraction()
        for setting in settings:
            dark_run_subtractor.add_setting(setting)

        # Create an actual scatter workspace
        transmission_workspace = self._get_transmission_workspace(trans_ids)

        # Execute the dark_run_subtractor - let exceptions flow to help track down errors
        transmission_workspace = dark_run_subtractor.execute_transmission(transmission_workspace, trans_ids)

        return transmission_workspace

    def _get_dark_file(self):
        # Provide an event file from the system test data repo
        return "SANS2D00028827_removed_spectra.nxs"

    def _get_sample_workspaces(self, run_number=None):
        monitor_ws = None
        file_path = None
        ws_name = None
        sample_ws = None
        if run_number is not None:
            file_path, ws_name = getFileAndName(run_number)
            alg_load = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load.initialize()
            alg_load.setChild(True)
            alg_load.setProperty("Filename", file_path)
            alg_load.setProperty("EntryNumber", 1)
            alg_load.setProperty("OutputWorkspace", ws_name)
            alg_load.execute()
            sample_ws = alg_load.getProperty("OutputWorkspace").value
        else:
            file_path, ws_name = getFileAndName(self._get_dark_file())
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
            monitors_name = ws_name + "_monitors"
            alg_load2 = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load2.initialize()
            alg_load2.setChild(True)
            alg_load2.setProperty("Filename", file_path)
            alg_load2.setProperty("EntryNumber", 2)
            alg_load2.setProperty("OutputWorkspace", monitors_name)
            alg_load2.execute()
            monitor_ws = alg_load2.getProperty("OutputWorkspace").value
        else:
            # Load the monitor workspace
            monitors_name = ws_name + "_monitors"
            alg_load_monitors = AlgorithmManager.createUnmanaged("LoadNexusMonitors")
            alg_load_monitors.initialize()
            alg_load_monitors.setChild(True)
            alg_load_monitors.setProperty("Filename", file_path)
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

    def _get_sample_workspace_histo(self):
        # Load the event workspace and the monitor workspace. Rebin the event to the monitor
        # and conjoin them
        file_path, ws_name = getFileAndName(self._get_dark_file())
        alg_load = AlgorithmManager.createUnmanaged("LoadEventNexus")
        alg_load.initialize()
        alg_load.setChild(True)
        alg_load.setProperty("Filename", file_path)
        alg_load.setProperty("OutputWorkspace", ws_name)
        alg_load.execute()
        event_ws = alg_load.getProperty("OutputWorkspace").value

        monitor_name = ws_name + "_monitor"
        alg_load2 = AlgorithmManager.createUnmanaged("LoadNexusMonitors")
        alg_load2.initialize()
        alg_load2.setChild(True)
        alg_load2.setProperty("Filename", file_path)
        alg_load2.setProperty("OutputWorkspace", monitor_name)
        alg_load2.execute()
        monitor_ws = alg_load2.getProperty("OutputWorkspace").value

        rebinned_name = ws_name + "_rebinned"
        alg_rebin = AlgorithmManager.createUnmanaged("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", event_ws)
        alg_rebin.setProperty("WorkspaceToMatch", monitor_ws)
        alg_rebin.setProperty("PreserveEvents", False)
        alg_rebin.setProperty("OutputWorkspace", rebinned_name)
        alg_rebin.execute()
        sample_ws = alg_rebin.getProperty("OutputWorkspace").value

        # We need to create a copy of the sample because it will be deleted/swallowed by
        # the conjoin below
        cloned_name = ws_name + "_cloned"
        alg_rebin = AlgorithmManager.createUnmanaged("CloneWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("InputWorkspace", sample_ws)
        alg_rebin.setProperty("OutputWorkspace", cloned_name)
        alg_rebin.execute()
        sample_ws_copy = alg_rebin.getProperty("OutputWorkspace").value

        alg_conjoined = AlgorithmManager.createUnmanaged("ConjoinWorkspaces")
        alg_conjoined.initialize()
        alg_conjoined.setChild(True)
        alg_conjoined.setProperty("InputWorkspace1", monitor_ws)
        alg_conjoined.setProperty("InputWorkspace2", sample_ws_copy)
        alg_conjoined.setProperty("CheckOverlapping", True)
        alg_conjoined.setProperty("CheckMatchingBins", False)
        alg_conjoined.execute()
        monitor_ws = alg_conjoined.getProperty("InputWorkspace1").value

        return sample_ws, monitor_ws

    def _get_transmission_workspace(self, trans_ids):
        trans_ws = None
        file_path = None

        file_path, ws_name = getFileAndName(self._get_dark_file())
        alg_load = AlgorithmManager.createUnmanaged("LoadEventNexus")
        alg_load.initialize()
        alg_load.setChild(True)
        alg_load.setProperty("Filename", file_path)
        alg_load.setProperty("OutputWorkspace", ws_name)
        alg_load.execute()
        detector = alg_load.getProperty("OutputWorkspace").value

        trans_name = ws_name + "_monitor"
        alg_load_monitors = AlgorithmManager.createUnmanaged("LoadNexusMonitors")
        alg_load_monitors.initialize()
        alg_load_monitors.setChild(True)
        alg_load_monitors.setProperty("Filename", file_path)
        alg_load_monitors.setProperty("OutputWorkspace", trans_name)
        alg_load_monitors.execute()
        monitor = alg_load_monitors.getProperty("OutputWorkspace").value

        rebinned_name = ws_name + "_rebinned"
        alg_rebin = AlgorithmManager.createUnmanaged("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", detector)
        alg_rebin.setProperty("WorkspaceToMatch", monitor)
        alg_rebin.setProperty("PreserveEvents", False)
        alg_rebin.setProperty("OutputWorkspace", rebinned_name)
        alg_rebin.execute()
        detector = alg_rebin.getProperty("OutputWorkspace").value

        alg_conjoined = AlgorithmManager.createUnmanaged("ConjoinWorkspaces")
        alg_conjoined.initialize()
        alg_conjoined.setChild(True)
        alg_conjoined.setProperty("InputWorkspace1", monitor)
        alg_conjoined.setProperty("InputWorkspace2", detector)
        alg_conjoined.setProperty("CheckOverlapping", True)
        alg_conjoined.setProperty("CheckMatchingBins", False)
        alg_conjoined.execute()
        trans_ws = alg_conjoined.getProperty("InputWorkspace1").value

        # And now extract all the required trans ids
        extracted_name = "extracted"
        alg_extract = AlgorithmManager.createUnmanaged("ExtractSpectra")
        alg_extract.initialize()
        alg_extract.setChild(True)
        alg_extract.setProperty("InputWorkspace", trans_ws)
        alg_extract.setProperty("OutputWorkspace", extracted_name)
        alg_extract.setProperty("DetectorList", trans_ids)
        alg_extract.execute()
        return alg_extract.getProperty("OutputWorkspace").value

    def _get_dark_run_settings_object(self, run, time, mean, use_mon, mon_number):
        # This is what would be coming from a parsed user file
        return DarkRunSettings(mon=use_mon, run_number=run, time=time, mean=mean, mon_number=mon_number)


class DarkRunSubtractionTestSystemTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(DarkRunSubtractionTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success
