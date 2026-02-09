# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from Engineering.common.instrument_config import CONFIGS, get_instr_config, SUPPORTED_INSTRUMENTS, GROUPS


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

    def test_enum_members_have_banks(self):
        for group in GROUPS:
            for m in group:
                self.assertTrue(hasattr(m, "banks"))


if __name__ == "__main__":
    unittest.main()
