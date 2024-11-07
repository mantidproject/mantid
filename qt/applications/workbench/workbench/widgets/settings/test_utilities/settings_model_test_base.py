# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from typing import Any, Callable, Sequence
from unittest import TestCase
from unittest.mock import MagicMock, call


class BaseSettingsModelTest(TestCase):
    def _test_getter_with_different_values(self, mock_obj: MagicMock, getter: Callable, values: Sequence[Any], mock_called_with: call):
        for value in values:
            mock_obj.return_value = value
            self.assertEqual(getter(), value)
            mock_obj.assert_has_calls([mock_called_with])
            mock_obj.reset_mock()

    @staticmethod
    def _test_setter_with_different_values(mock_obj: MagicMock, setter: Callable, values: Sequence[Any], property_string: str):
        for value in values:
            setter(value)
            mock_obj.assert_has_calls([call(property_string, value)])
            mock_obj.reset_mock()
