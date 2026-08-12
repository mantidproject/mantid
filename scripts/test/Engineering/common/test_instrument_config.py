# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from Engineering.common.instrument_config import (
    CONFIGS,
    GROUPS,
    SUPPORTED_INSTRUMENTS,
    get_cube_preset_extent,
    get_gauge_volume_presets,
    get_instr_config,
)


class InstrumentConfigTest(unittest.TestCase):
    def test_get_instr_config_returns_none_for_none(self):
        self.assertIsNone(get_instr_config(None))

    def test_get_instr_config_case_insensitive(self):
        test_cases = (("enginx", "ENGINX"), ("EnginX", "ENGINX"), ("imat", "IMAT"), ("IMAT", "IMAT"))

        for input, expected in test_cases:
            with self.subTest(input=input, expected=expected):
                self.assertIs(get_instr_config(input), CONFIGS[expected])

    def test_get_instr_config_raises_for_unknown(self):
        with self.assertRaisesRegex(RuntimeError, r"No instrument config registered"):
            get_instr_config("NOT_AN_INSTRUMENT")

    def test_configs_contains_expected_keys(self):
        for instr in SUPPORTED_INSTRUMENTS:
            self.assertIn(instr, CONFIGS)

    def test_each_instr_has_supporting_group(self):
        for instr in SUPPORTED_INSTRUMENTS:
            group = get_instr_config(instr).group
            self.assertTrue(group in GROUPS)

    def test_each_group_member_has_group_info_with_banks(self):
        for instr in SUPPORTED_INSTRUMENTS:
            config = get_instr_config(instr)
            for member in config.group:
                with self.subTest(instr=instr, member=member):
                    self.assertIn(member, config.group_info)
                    self.assertTrue(hasattr(config.group_info[member], "banks"))


class GaugeVolumePresetTest(unittest.TestCase):
    ENGINX_PRESETS = ("0.5mmCube", "1mmCube", "2mmCube", "3mmCube", "4mmCube")

    def test_enginx_presets_cover_every_collimator_in_size_order(self):
        self.assertEqual(get_gauge_volume_presets("ENGINX"), self.ENGINX_PRESETS)

    def test_presets_looked_up_by_pseudonym_and_case(self):
        for instrument in ("ENGIN-X", "enginx", "EnginX"):
            with self.subTest(instrument=instrument):
                self.assertEqual(get_gauge_volume_presets(instrument), self.ENGINX_PRESETS)

    def test_instrument_without_configured_optics_still_offers_cube_presets(self):
        self.assertEqual(get_gauge_volume_presets("IMAT"), self.ENGINX_PRESETS)

    def test_unknown_instrument_still_offers_cube_presets(self):
        for instrument in (None, "NOT_AN_INSTRUMENT"):
            with self.subTest(instrument=instrument):
                self.assertEqual(get_gauge_volume_presets(instrument), self.ENGINX_PRESETS)

    def test_every_preset_names_a_cube_of_its_own_size(self):
        for instr in SUPPORTED_INSTRUMENTS:
            for preset in get_gauge_volume_presets(instr):
                with self.subTest(instr=instr, preset=preset):
                    self.assertIsNotNone(get_cube_preset_extent(preset))

    def test_get_cube_preset_extent_converts_name_to_metres(self):
        for preset, expected in (("4mmCube", 0.004), ("0.5mmCube", 0.0005), ("1mmCube", 0.001)):
            with self.subTest(preset=preset):
                self.assertAlmostEqual(get_cube_preset_extent(preset), expected)

    def test_get_cube_preset_extent_none_for_non_cube_presets(self):
        for preset in (None, "Custom Shape", "No Gauge Volume", "4mm", "mmCube"):
            with self.subTest(preset=preset):
                self.assertIsNone(get_cube_preset_extent(preset))


if __name__ == "__main__":
    unittest.main()
