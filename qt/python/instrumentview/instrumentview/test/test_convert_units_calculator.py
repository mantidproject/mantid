# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.ConvertUnitsCalculator import ConvertUnitsCalculator
from mantid.simpleapi import CreateSampleWorkspace
import unittest
from unittest.mock import patch


class TestConvertUnitsCalculator(unittest.TestCase):
    @classmethod
    def setUp(cls) -> None:
        workspace = CreateSampleWorkspace(EnableLogging=False)
        cls.workspace = workspace
        cls.calculator = ConvertUnitsCalculator(workspace)
        cls.units_to_test = ["TOF", "dSpacing", "MomentumTransfer", "Wavelength"]

    @patch("mantid.kernel.UnitConversion.run")
    def test_convert_same_unit(self, mock_run_conversion):
        test_value = 10
        for unit in self.units_to_test:
            converted = self.calculator.convert(unit, unit, 101, test_value)
            self.assertEqual(test_value, converted)
            mock_run_conversion.assert_not_called()

    @patch("mantid.kernel.UnitConversion.run")
    def test_run_conversions(self, mock_run_conversion):
        test_value = 10
        for source_unit in self.units_to_test:
            for target_unit in self.units_to_test:
                if source_unit == target_unit:
                    continue
                self.calculator.convert(source_unit, target_unit, 101, test_value)
                mock_run_conversion.assert_called_once()
                mock_run_conversion.reset_mock()


if __name__ == "__main__":
    unittest.main()
