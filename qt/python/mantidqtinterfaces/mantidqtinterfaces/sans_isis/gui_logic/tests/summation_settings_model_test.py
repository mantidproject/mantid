# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import BinningType
from mantidqtinterfaces.sans_isis.gui_logic.models.SummationSettingsModel import SummationSettingsModel


class SummationSettingsTestCase(unittest.TestCase):
    def setUpWithInitialType(self, initial_type):
        self.summation_settings = SummationSettingsModel(initial_type)

    def setUp(self):
        self.setUpWithInitialType(BinningType.CUSTOM)


class SummationSettingsOverlayEventWorkspaceTestCase(unittest.TestCase):
    def assertHasOverlayEventWorkspaces(self):
        self.assertTrue(self.summation_settings.has_overlay_event_workspaces())

    def assertDoesNotHaveOverlayEventWorkspaces(self):
        self.assertFalse(self.summation_settings.has_overlay_event_workspaces())

    def assertOverlayEventWorkspacesDisabled(self):
        self.assertFalse(self.summation_settings.is_overlay_event_workspaces_enabled())

    def assertOverlayEventWorkspacesEnabled(self):
        self.assertTrue(self.summation_settings.is_overlay_event_workspaces_enabled())


class SummationSettingsBinSettingsTest(SummationSettingsTestCase):
    def assertHasBinSettings(self):
        self.assertTrue(self.summation_settings.has_bin_settings())

    def assertDoesNotHaveBinSettings(self):
        self.assertFalse(self.summation_settings.has_bin_settings())

    def test_can_set_bin_settings_when_in_custom(self):
        bin_settings = "1,24,545,23"
        self.summation_settings.bin_settings = bin_settings
        self.assertEqual(bin_settings, self.summation_settings.bin_settings)

    def test_custom_binning_has_bin_settings(self):
        self.setUpWithInitialType(BinningType.CUSTOM)
        self.assertHasBinSettings()

    def test_save_as_event_data_does_not_have_bin_settings(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.assertDoesNotHaveBinSettings()

    def test_from_monitors_does_not_have_bin_settings(self):
        self.setUpWithInitialType(BinningType.FROM_MONITORS)
        self.assertDoesNotHaveBinSettings()


class SummationSettingsAdditionalTimeShiftsTest(SummationSettingsTestCase, SummationSettingsOverlayEventWorkspaceTestCase):
    def assertHasAdditionalTimeShifts(self):
        self.assertTrue(self.summation_settings.has_additional_time_shifts())

    def assertDoesNotHaveAdditionalTimeShifts(self):
        self.assertFalse(self.summation_settings.has_additional_time_shifts())

    def test_custom_binning_does_not_have_additional_time_shifts(self):
        self.setUpWithInitialType(BinningType.CUSTOM)
        self.assertDoesNotHaveAdditionalTimeShifts()

    def test_save_as_event_data_has_additional_time_shifts_if_overlay_event_workspaces_enabled(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.assertDoesNotHaveAdditionalTimeShifts()
        self.summation_settings.enable_overlay_event_workspaces()
        self.assertHasAdditionalTimeShifts()
        self.summation_settings.disable_overlay_event_workspaces()
        self.assertDoesNotHaveAdditionalTimeShifts()

    def test_from_monitors_does_not_have_additional_time_shifts(self):
        self.setUpWithInitialType(BinningType.FROM_MONITORS)
        self.assertDoesNotHaveAdditionalTimeShifts()

    def test_can_set_additional_time_shifts_when_available(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.summation_settings.enable_overlay_event_workspaces()
        additional_time_shifts = "1,24,545,23"
        self.summation_settings.additional_time_shifts = additional_time_shifts
        self.assertEqual(additional_time_shifts, self.summation_settings.additional_time_shifts)

    def test_stores_additional_time_shifts_between_mode_switches(self):
        bin_settings = "232,2132,123"
        additional_time_shifts = "32,252,12"
        self.setUpWithInitialType(BinningType.CUSTOM)
        self.summation_settings.bin_settings = bin_settings
        self.summation_settings.set_histogram_binning_type(BinningType.SAVE_AS_EVENT_DATA)
        self.summation_settings.additional_time_shifts = additional_time_shifts
        self.summation_settings.set_histogram_binning_type(BinningType.CUSTOM)
        self.assertEqual(bin_settings, self.summation_settings.bin_settings)
        self.summation_settings.set_histogram_binning_type(BinningType.SAVE_AS_EVENT_DATA)
        self.assertEqual(additional_time_shifts, self.summation_settings.additional_time_shifts)


class SummationSettingsOverlayEventWorkspace(SummationSettingsTestCase, SummationSettingsOverlayEventWorkspaceTestCase):
    def test_custom_binning_does_not_have_overlay_event_workspaces(self):
        self.setUpWithInitialType(BinningType.CUSTOM)
        self.assertDoesNotHaveOverlayEventWorkspaces()

    def test_save_as_event_data_has_overlay_event_workspaces(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.assertHasOverlayEventWorkspaces()

    def test_from_monitors_does_not_have_overlay_event_workspaces(self):
        self.setUpWithInitialType(BinningType.FROM_MONITORS)
        self.assertDoesNotHaveOverlayEventWorkspaces()

    def test_switching_to_save_as_event_data_enables_overlay_event_workspaces_option(self):
        self.setUpWithInitialType(BinningType.FROM_MONITORS)
        self.summation_settings.set_histogram_binning_type(BinningType.SAVE_AS_EVENT_DATA)
        self.assertHasOverlayEventWorkspaces()

    def test_can_enable_overlay_event_workspaces_when_available(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.summation_settings.enable_overlay_event_workspaces()
        self.assertOverlayEventWorkspacesEnabled()

    def test_can_disable_overlay_event_workspaces_when_available(self):
        self.setUpWithInitialType(BinningType.SAVE_AS_EVENT_DATA)
        self.summation_settings.enable_overlay_event_workspaces()
        self.summation_settings.disable_overlay_event_workspaces()
        self.assertOverlayEventWorkspacesDisabled()


if __name__ == "__main__":
    unittest.main()
