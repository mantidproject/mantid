# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import unittest
from unittest import TestCase
from unittest.mock import MagicMock, patch

from mantid.simpleapi import CreateSampleWorkspace, CropWorkspace, mtd, ConvertToHistogram, AssertSpinStateOrder
from mantid.api import WorkspaceGroup


def create_fake_pol_sans_ws(name: str):
    ws_group = WorkspaceGroup()
    mtd.add(name, ws_group)
    y_values = [10, 80, 20, 90]

    for i in range(4):
        ws_name = f"ws_{i}"
        CreateSampleWorkspace(
            "Histogram",
            "Flat background",
            XUnit="Wavelength",
            XMin=0,
            XMax=10,
            BinWidth=1,
            NumEvents=10,
            InstrumentName="LARMOR",
            OutputWorkspace=ws_name,
        )
        CropWorkspace(ws_name, StartWorkspaceIndex=1, EndWorkspaceIndex=1, OutputWorkspace=ws_name)
        mtd[ws_name] *= y_values[i]
        ws_group.addWorkspace(mtd[ws_name])
    ConvertToHistogram(ws_group, OutputWorkspace=ws_group)
    return ws_group


@patch("plugins.algorithms.AssertSpinStateOrder.DetermineSpinStateOrder")
class AssertSpinStateOrderTest(TestCase):
    def setUp(self):
        self.group_ws_name = "ws_group"
        self.group_ws = create_fake_pol_sans_ws("ws_group")

    def tearDown(self):
        mtd.clear()

    def test_returns_true_for_matching_spin_states(self, mock_DetermineSpinStateOrder: MagicMock):
        spin_states = "00,01,10,11"
        mock_DetermineSpinStateOrder.return_value = spin_states
        result = AssertSpinStateOrder(self.group_ws, spin_states)
        self.assertTrue(result)

    def test_returns_false_for_mismatching_spin_states(self, mock_DetermineSpinStateOrder: MagicMock):
        mock_DetermineSpinStateOrder.return_value = "00,01,10,11"
        result = AssertSpinStateOrder(self.group_ws, "01,00,11,10")
        self.assertFalse(result)

    def test_reorder_true_reorders_the_workspace_when_orders_mismatch(self, mock_DetermineSpinStateOrder: MagicMock):
        mock_DetermineSpinStateOrder.return_value = "00,01,10,11"
        self.assertEqual(list(self.group_ws.getNames()), ["ws_0", "ws_1", "ws_2", "ws_3"])
        AssertSpinStateOrder(self.group_ws, "01,00,11,10", True)
        self.assertEqual(list(mtd[self.group_ws_name].getNames()), ["ws_1", "ws_0", "ws_3", "ws_2"])

    @patch("mantid.kernel.logger.warning")
    def test_warning_is_logged_when_orders_mismatch(self, mock_logger_warning: MagicMock, mock_DetermineSpinStateOrder: MagicMock):
        returned_spin_state_order = "00,01,10,11"
        expected_spin_state_order = "01,00,11,10"
        mock_DetermineSpinStateOrder.return_value = returned_spin_state_order
        AssertSpinStateOrder(self.group_ws, expected_spin_state_order)
        mock_logger_warning.assert_called_once_with(
            f"Expected {self.group_ws_name} to have the spin state order '{expected_spin_state_order}' "
            f"but actually found '{returned_spin_state_order}'"
        )

    @patch("mantid.kernel.logger.warning")
    def test_warning_is_logged_when_workspace_is_reordered(self, mock_logger_warning: MagicMock, mock_DetermineSpinStateOrder: MagicMock):
        mock_DetermineSpinStateOrder.return_value = "00,01,10,11"
        expected_spin_state_order = "01,00,11,10"
        AssertSpinStateOrder(self.group_ws, expected_spin_state_order, True)
        mock_logger_warning.assert_called_with(
            f"Reordered {self.group_ws_name} to the correct spin state order '{expected_spin_state_order}'"
        )


if __name__ == "__main__":
    unittest.main()
