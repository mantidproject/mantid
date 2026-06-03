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

file_path = "mantidqtinterfaces.TexturePlanner.helpers.instrument"


def _make_model():
    return MagicMock()


@patch(file_path + ".get_instr_config")
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


@patch(file_path + ".get_instr_config")
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

    def test_supported_groups_for_known_instruments(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")

        helper.update_instrument("IMAT")
        self.assertEqual(helper.supported_groups, ("Module1", "Module4", "Row1", "Row4", "banks"))

        helper.update_instrument("ENGINX")
        self.assertEqual(helper.supported_groups, ("Texture20", "Texture30", "banks"))

    def test_supported_groups_falls_back_for_unknown_instrument(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")

        helper.update_instrument("UNKNOWN_INSTR")

        self.assertEqual(helper.supported_groups, ("banks",))


@patch(file_path + ".get_instr_config")
class TestInstrumentHelper_GroupsAndInstruments(unittest.TestCase):
    def test_get_instrument_returns_current_instr(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="IMAT")

        self.assertEqual(helper.get_instrument(), "IMAT")

    def test_get_supported_instruments_delegates(self, mock_get_cfg):
        with patch(file_path + ".SUPPORTED_INSTRUMENTS", ("ENGINX", "IMAT")):
            self.assertEqual(InstrumentHelper.get_supported_instruments(), ("ENGINX", "IMAT"))

    def test_set_group_resolves_group_from_config(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.config = MagicMock()
        helper.config.group.return_value = "grp_obj"

        helper.set_group("Texture20")

        helper.config.group.assert_called_once_with("Texture20")
        self.assertEqual(helper.group, "grp_obj")


@patch(file_path + ".get_instr_config")
class TestInstrumentHelper_GroupingPath(unittest.TestCase):
    def test_get_grouping_file_reads_from_config(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.group = "grp_obj"
        helper.config = MagicMock()
        helper.config.grouping_files = {"grp_obj": "GRP.xml"}

        self.assertEqual(helper.get_grouping_file(), "GRP.xml")

    @patch(file_path + ".CALIB_DIR", "/calib")
    def test_get_grouping_path_joins_calib_dir(self, mock_get_cfg):
        helper = InstrumentHelper(_make_model(), instrument="ENGINX")
        helper.get_grouping_file = MagicMock(return_value="GRP.xml")

        self.assertEqual(helper.get_grouping_path(), os.path.join("/calib", "GRP.xml"))


if __name__ == "__main__":
    unittest.main()
