# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from unittest.mock import patch, MagicMock

from mantidqtinterfaces.TexturePlanner.helpers.instrument import InstrumentHelper

FILE_PATH = "mantidqtinterfaces.TexturePlanner.helpers.instrument"


def _make_model():
    return MagicMock()


@patch(FILE_PATH + ".get_instr_config")
class TestInstrumentHelper_Init(unittest.TestCase):
    def test_sets_defaults_without_bootstrapping(self, mock_get_cfg):
        # The model binds the helper to itself before bootstrapping via update_instrument,
        # so construction alone must not touch the config or the workspaces.
        model = _make_model()

        helper = InstrumentHelper(model, instrument="ENGINX")

        self.assertEqual(helper.instr, "ENGINX")
        self.assertIsNone(helper.config)
        self.assertIsNone(helper.group)
        self.assertEqual(helper.supported_groups, ("banks",))
        mock_get_cfg.assert_not_called()
        model.workspaces.update_ws.assert_not_called()

    def test_defaults_to_enginx(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model())

        self.assertEqual(helper.instr, "ENGINX")


@patch(FILE_PATH + ".get_instr_config")
class TestInstrumentHelper_UpdateInstrument(unittest.TestCase):
    def test_updates_instr_config_and_workspaces(self, mock_get_cfg):
        model = _make_model()
        helper = InstrumentHelper(model, instrument="ENGINX")
        mock_get_cfg.reset_mock()
        model.workspaces.update_ws.reset_mock()

        helper.update_instrument("IMAT")

        self.assertEqual(helper.instr, "IMAT")
        mock_get_cfg.assert_called_once_with("IMAT")
        self.assertEqual(helper.config, mock_get_cfg.return_value)
        model.workspaces.update_ws.assert_called_once_with()

    def test_groups_for_instrument_does_not_apply(self, mock_get_cfg):
        # used by the view to repopulate the group combo on selection, without mutating the model
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        mock_get_cfg.reset_mock()

        self.assertEqual(helper.groups_for_instrument("IMAT"), ("Module1", "Module4", "Row1", "Row4", "banks", "Custom"))
        self.assertEqual(helper.groups_for_instrument("ANYTHING_ELSE"), ("Custom",))
        # querying the options must not switch the active instrument or touch the config/workspaces
        self.assertEqual(helper.instr, "ENGINX")
        mock_get_cfg.assert_not_called()

    def test_supported_groups_for_known_instruments(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")

        helper.update_instrument("IMAT")
        self.assertEqual(helper.supported_groups, ("Module1", "Module4", "Row1", "Row4", "banks", "Custom"))

        helper.update_instrument("ENGINX")
        self.assertEqual(helper.supported_groups, ("Texture20", "Texture30", "banks", "Custom"))

    def test_unknown_instrument_is_treated_as_custom(self, mock_get_cfg):
        # custom instruments have no registered config and only offer the custom grouping option
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        mock_get_cfg.reset_mock()

        helper.update_instrument("UNKNOWN_INSTR")

        self.assertEqual(helper.supported_groups, ("Custom",))
        self.assertIsNone(helper.config)
        mock_get_cfg.assert_not_called()


@patch(FILE_PATH + ".get_instr_config")
class TestInstrumentHelper_GroupsAndInstruments(unittest.TestCase):
    def test_get_instrument_returns_current_instr(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="IMAT")

        self.assertEqual(helper.get_instrument(), "IMAT")

    def test_get_supported_instruments_delegates(self, mock_get_cfg):
        with patch(FILE_PATH + ".SUPPORTED_INSTRUMENTS", ("ENGINX", "IMAT")):
            self.assertEqual(InstrumentHelper.get_supported_instruments(), ("ENGINX", "IMAT"))

    def test_set_group_resolves_group_from_config(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.config = MagicMock()
        helper.config.group.return_value = "grp_obj"

        helper.set_group("Texture20")

        helper.config.group.assert_called_once_with("Texture20")
        self.assertEqual(helper.group, "grp_obj")

    def test_set_group_custom_uses_sentinel_without_config(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.config = MagicMock()

        helper.set_group("Custom")

        helper.config.group.assert_not_called()
        self.assertEqual(helper.group, "Custom")


@patch(FILE_PATH + ".get_instr_config")
class TestInstrumentHelper_GroupingPath(unittest.TestCase):
    def test_get_grouping_file_reads_from_config(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.group = "grp_obj"
        helper.config = MagicMock()
        helper.config.grouping_files = {"grp_obj": "GRP.xml"}

        self.assertEqual(helper.get_grouping_file(), "GRP.xml")

    @patch(FILE_PATH + ".CALIB_DIR", "/calib")
    def test_get_grouping_path_joins_calib_dir(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.get_grouping_file = MagicMock(return_value="GRP.xml")

        self.assertEqual(helper.get_grouping_path(), os.path.join("/calib", "GRP.xml"))

    def test_get_grouping_path_returns_custom_file_for_custom_group(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.set_custom_grouping_file("/abs/path/custom_grouping.xml")
        helper.group = "Custom"

        self.assertEqual(helper.get_grouping_path(), "/abs/path/custom_grouping.xml")


@patch(FILE_PATH + ".GroupDetectors")
@patch(FILE_PATH + ".CreateSimulationWorkspace")
class TestInstrumentHelper_IsGroupingFileApplicable(unittest.TestCase):
    @staticmethod
    def _grouped_with(has_detectors):
        grouped = MagicMock()
        grouped.getNumberHistograms.return_value = len(has_detectors)
        grouped.spectrumInfo.return_value.hasDetectors.side_effect = lambda i: has_detectors[i]
        return grouped

    def test_false_without_lookup_for_missing_inputs(self, mock_sim, mock_group):
        self.assertFalse(InstrumentHelper.is_grouping_file_applicable("", "/grp.xml"))
        self.assertFalse(InstrumentHelper.is_grouping_file_applicable("WISH", ""))
        mock_sim.assert_not_called()
        mock_group.assert_not_called()

    def test_true_when_all_groups_have_detectors(self, mock_sim, mock_group):
        mock_group.return_value = self._grouped_with([True, True, True])

        self.assertTrue(InstrumentHelper.is_grouping_file_applicable("WISH", "/grp.xml"))
        mock_sim.assert_called_once()
        mock_group.assert_called_once()

    def test_true_when_only_a_leading_null_group_is_empty(self, mock_sim, mock_group):
        mock_group.return_value = self._grouped_with([False, True, True])

        self.assertTrue(InstrumentHelper.is_grouping_file_applicable("WISH", "/grp.xml"))

    def test_false_when_a_non_leading_group_is_empty(self, mock_sim, mock_group):
        mock_group.return_value = self._grouped_with([True, False, True])

        self.assertFalse(InstrumentHelper.is_grouping_file_applicable("WISH", "/grp.xml"))

    def test_false_when_all_groups_are_empty(self, mock_sim, mock_group):
        mock_group.return_value = self._grouped_with([False, False])

        self.assertFalse(InstrumentHelper.is_grouping_file_applicable("WISH", "/grp.xml"))

    def test_false_when_grouping_raises(self, mock_sim, mock_group):
        mock_group.side_effect = RuntimeError("incompatible grouping")

        self.assertFalse(InstrumentHelper.is_grouping_file_applicable("WISH", "/grp.xml"))


@patch(FILE_PATH + ".InstrumentFileFinder")
class TestInstrumentHelper_IsValidInstrument(unittest.TestCase):
    def test_true_when_idf_found(self, mock_finder):
        mock_finder.getInstrumentFilename.return_value = "/instr/WISH_Definition.xml"

        self.assertTrue(InstrumentHelper.is_valid_instrument("WISH"))
        mock_finder.getInstrumentFilename.assert_called_once_with("WISH")

    def test_false_when_no_idf(self, mock_finder):
        mock_finder.getInstrumentFilename.return_value = ""

        self.assertFalse(InstrumentHelper.is_valid_instrument("NOTREAL"))

    def test_false_for_empty_name_without_lookup(self, mock_finder):
        self.assertFalse(InstrumentHelper.is_valid_instrument(""))
        mock_finder.getInstrumentFilename.assert_not_called()


if __name__ == "__main__":
    unittest.main()
