# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import unittest

from qtpy import QtCore
from qtpy.QtCore import Qt

from unittest.mock import MagicMock, Mock, call
from mantidqt.utils.testing.mocks.mock_mantid import (
    AXIS_INDEX_FOR_HORIZONTAL,
    AXIS_INDEX_FOR_VERTICAL,
    MockMantidAxis,
    MockMantidSymbol,
    MockMantidUnit,
    MockSpectrum,
    MockWorkspace,
)
from mantidqt.utils.testing.mocks.mock_qt import MockQModelIndex
from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModel, MatrixWorkspaceTableViewModelType
from mantid.simpleapi import CreateWorkspace


def setup_common_for_test_data():
    """
    Common configuration of variables and mocking for testing
    MatrixWorkspaceDisplayTableViewModel's data and headerData functions
    """
    # Create some mock data for the mock workspace
    row = 2
    column = 2
    # make a workspace with 0s
    mock_data = [0] * 10
    # set one of them to be not 0
    mock_data[column] = 999
    model_type = MatrixWorkspaceTableViewModelType.x
    # pass onto the MockWorkspace so that it returns it when read from the TableViewModel
    ws = MockWorkspace(read_return=mock_data)
    ws.hasMaskedBins = Mock(return_value=True)
    ws.maskedBinsIndices = Mock(return_value=[column])
    model = MatrixWorkspaceTableViewModel(ws, model_type)
    # The model retrieves the spectrumInfo object, and our MockWorkspace has already given it
    # the MockSpectrumInfo, so all that needs to be done here is to set up the correct method Mocks
    model.ws_spectrum_info.hasDetectors = Mock(return_value=True)
    index = MockQModelIndex(row, column)
    return ws, model, row, index


class MatrixWorkspaceDisplayTableViewModelTest(unittest.TestCase):
    def test_correct_model_type(self):
        ws = MockWorkspace()
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.x)
        self.assertEqual(model.type, MatrixWorkspaceTableViewModelType.x)

        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.y)
        self.assertEqual(model.type, MatrixWorkspaceTableViewModelType.y)

        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.e)
        self.assertEqual(model.type, MatrixWorkspaceTableViewModelType.e)

    def test_correct_cell_colors(self):
        ws = MockWorkspace()
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.x)
        self.assertEqual((240, 240, 240, 255), model.masked_color.getRgb())
        self.assertEqual((255, 253, 209, 255), model.monitor_color.getRgb())

    def test_correct_relevant_data(self):
        ws = MockWorkspace()
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.x)
        msg = "The function is not set correctly! The wrong data will be read."
        self.assertEqual(ws.readX, model.relevant_data, msg=msg)
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.y)
        self.assertEqual(ws.readY, model.relevant_data, msg=msg)
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.e)
        self.assertEqual(ws.readE, model.relevant_data, msg=msg)

    def test_invalid_model_type(self):
        ws = MockWorkspace()
        with self.assertRaises(AssertionError):
            MatrixWorkspaceTableViewModel(ws, "My Model Type")

    def test_data_display_role(self):
        # Create some mock data for the mock workspace
        row = 2
        column = 2
        # make a workspace with 0s
        mock_data = [0] * 10
        # set one of them to be not 0
        mock_data[column] = 999
        # pass onto the MockWorkspace so that it returns it when read from the TableViewModel
        self._check_correct_data_is_displayed(MatrixWorkspaceTableViewModelType.x, column, mock_data, row)
        self._check_correct_data_is_displayed(MatrixWorkspaceTableViewModelType.y, column, mock_data, row)
        self._check_correct_data_is_displayed(MatrixWorkspaceTableViewModelType.e, column, mock_data, row)

    def _check_correct_data_is_displayed(self, model_type, column, mock_data, row):
        ws = MockWorkspace(read_return=mock_data)
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        index = MockQModelIndex(row, column)
        output = model.data(index, Qt.DisplayRole)
        model.relevant_data.assert_called_with(row)
        self.assertEqual(str(mock_data[column]), output)

    def test_row_and_column_count(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        MatrixWorkspaceTableViewModel(ws, model_type)
        # these are called when the TableViewModel is initialised
        ws.getNumberHistograms.assert_called()

    def test_data_background_role_masked_row(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=True)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_once_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)

        index.row.assert_called_once_with()
        index.column.assert_called_once_with()

        self.assertEqual(model.masked_color, output)

        # Just do it a second time -> This time it's cached and should be read off the cache.
        # If it is not read off the cache the assert_called_once below will fail,
        # as the functions would be called a 2nd time
        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_once_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        self.assertEqual(model.masked_color, output)

        # assert that the row was called twice with no parameters
        self.assertEqual(2, index.row.call_count)
        self.assertEqual(2, index.column.call_count)

    def test_data_background_role_monitor_row(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=True)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        index.row.assert_called_once()
        index.column.assert_called_once()

        self.assertEqual(model.monitor_color, output)

        # Just do it a second time -> This time it's cached and should be read off the cache.
        # If it is not read off the cache the assert_called_once below will fail,
        # as the functions would be called a 2nd time
        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_with(row)
        # assert that it has been called twice with the same parameters
        model.ws_spectrum_info.isMasked.assert_has_calls([call.do_work(row), call.do_work(row)])
        # only called once, as the 2nd time should have hit the cached monitor
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)
        self.assertEqual(model.monitor_color, output)

        # assert that the row was called twice with no parameters
        self.assertEqual(2, index.row.call_count)
        self.assertEqual(2, index.column.call_count)

    def test_data_background_role_masked_bin(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=False)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)
        ws.hasMaskedBins.assert_called_once_with(row)
        ws.maskedBinsIndices.assert_called_once_with(row)

        index.row.assert_called_once()
        index.column.assert_called_once()

        self.assertEqual(model.masked_color, output)
        # Just do it a second time -> This time it's cached and should be read off the cache.
        # If it is not read off the cache the assert_called_once below will fail,
        # as the functions would be called a 2nd time
        output = model.data(index, Qt.BackgroundRole)

        # masked bins is checked last, so it will call all other functions a second time
        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMonitor.assert_has_calls([call.do_work(row), call.do_work(row)])

        # these, however, should remain at 1 call, as the masked bin cache should have been hit
        ws.hasMaskedBins.assert_called_once_with(row)
        ws.maskedBinsIndices.assert_called_once_with(row)

        self.assertEqual(model.masked_color, output)

        self.assertEqual(2, index.row.call_count)
        self.assertEqual(2, index.column.call_count)

    def test_data_tooltip_role_masked_row(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.hasDetectors = Mock(return_value=True)
        model.ws_spectrum_info.isMasked = Mock(return_value=True)
        model.ws_spectrum_info.isMonitor = Mock(return_value=False)

        output = model.data(index, Qt.ToolTipRole)

        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_ROW_TOOLTIP, output)

        output = model.data(index, Qt.ToolTipRole)

        # The row was masked so it should have been cached
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)

        # However it is checked if it is a monitor again
        self.assertEqual(3, model.ws_spectrum_info.hasDetectors.call_count)
        self.assertEqual(2, model.ws_spectrum_info.isMonitor.call_count)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_ROW_TOOLTIP, output)

    def test_data_tooltip_role_masked_monitor_row(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=True)
        model.ws_spectrum_info.isMonitor = Mock(return_value=True)

        output = model.data(index, Qt.ToolTipRole)

        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_MONITOR_ROW_TOOLTIP, output)

        # Doing the same thing a second time should hit the cache, so no additional calls will have been made
        output = model.data(index, Qt.ToolTipRole)
        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_MONITOR_ROW_TOOLTIP, output)

    def test_data_tooltip_role_monitor_row(self):
        ws, model, row, index = setup_common_for_test_data()

        # necessary otherwise it is returned that there is a masked bin, and we get the wrong output
        ws.hasMaskedBins = Mock(return_value=False)

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=True)

        output = model.data(index, Qt.ToolTipRole)

        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MONITOR_ROW_TOOLTIP, output)

        # Doing the same thing a second time should hit the cache, so no additional calls will have been made
        output = model.data(index, Qt.ToolTipRole)
        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        self.assertEqual(2, model.ws_spectrum_info.isMasked.call_count)
        # This was called only once because the monitor was cached
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MONITOR_ROW_TOOLTIP, output)

    def test_data_tooltip_role_masked_bin_in_monitor_row(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=True)
        ws.hasMaskedBins = Mock(return_value=True)
        ws.maskedBinsIndices = Mock(return_value=[index.column()])

        output = model.data(index, Qt.ToolTipRole)

        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MONITOR_ROW_TOOLTIP + MatrixWorkspaceTableViewModel.MASKED_BIN_TOOLTIP, output)

        # Doing the same thing a second time should hit the cache, so no additional calls will have been made
        output = model.data(index, Qt.ToolTipRole)
        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        self.assertEqual(2, model.ws_spectrum_info.isMasked.call_count)
        # This was called only once because the monitor was cached
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MONITOR_ROW_TOOLTIP + MatrixWorkspaceTableViewModel.MASKED_BIN_TOOLTIP, output)

    def test_data_tooltip_role_masked_bin(self):
        ws, model, row, index = setup_common_for_test_data()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=False)
        ws.hasMaskedBins = Mock(return_value=True)
        ws.maskedBinsIndices = Mock(return_value=[index.column()])

        output = model.data(index, Qt.ToolTipRole)

        model.ws_spectrum_info.hasDetectors.assert_has_calls([call.do_work(row), call.do_work(row)])
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_BIN_TOOLTIP, output)

        # Doing the same thing a second time should hit the cache, so no additional calls will have been made
        output = model.data(index, Qt.ToolTipRole)
        self.assertEqual(4, model.ws_spectrum_info.hasDetectors.call_count)
        self.assertEqual(2, model.ws_spectrum_info.isMasked.call_count)
        self.assertEqual(2, model.ws_spectrum_info.isMonitor.call_count)
        # This was called only once because the monitor was cached
        ws.hasMaskedBins.assert_called_once_with(row)
        ws.maskedBinsIndices.assert_called_once_with(row)

        self.assertEqual(MatrixWorkspaceTableViewModel.MASKED_BIN_TOOLTIP, output)

    def test_headerData_not_display_or_tooltip(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(0, Qt.Vertical, Qt.BackgroundRole)
        self.assertTrue(isinstance(output, QtCore.QVariant))

    def test_headerData_vertical_header_display_role(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.DisplayRole)

        ws.getAxis.assert_called_with(AXIS_INDEX_FOR_VERTICAL)
        ws.mock_axis.label.assert_called_once_with(mock_section)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_DISPLAY_STRING.format(mock_section, MockMantidAxis.TEST_LABEL)

        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_tooltip_role(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.ToolTipRole)

        ws.getSpectrum.assert_called_once_with(mock_section)
        ws.mock_spectrum.getSpectrumNo.assert_called_once_with()

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_TOOLTIP_STRING.format(mock_section, MockSpectrum.SPECTRUM_NO)
        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_display_for_numeric_axis_with_point_data(self):
        dummy_unit = "unit"
        ws = MockWorkspace()
        mock_axis = Mock()
        mock_axis.isNumeric.return_value = True
        expected_value = 0.0
        mock_axis.label = MagicMock(side_effect=[str(expected_value)])
        mock_axis.getUnit().symbol().utf8.return_value = dummy_unit
        ws.getAxis.return_value = mock_axis

        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.DisplayRole)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_DISPLAY_STRING_FOR_NUMERIC_AXIS.format(
            mock_section, expected_value, dummy_unit
        )
        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_display_for_numeric_axis_with_binned_data(self):
        dummy_unit = "unit"
        ws = MockWorkspace()
        mock_axis = Mock()
        mock_axis.isNumeric.return_value = True
        # single spectrum with length 2 axis
        ws.getNumberHistograms.return_value = 1
        mock_axis.length.return_value = 2
        mock_axis.label = MagicMock(side_effect=["0.5"])
        mock_axis.getUnit().symbol().utf8.return_value = dummy_unit
        ws.getAxis.return_value = mock_axis

        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.DisplayRole)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_DISPLAY_STRING_FOR_NUMERIC_AXIS.format(
            mock_section, 0.5, dummy_unit
        )
        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_tooltip_for_numeric_axis_with_point_data(self):
        dummy_unit = "unit"
        ws = MockWorkspace()
        mock_axis = Mock()
        mock_axis.isNumeric.return_value = True
        ws.getNumberHistograms.return_value = 1
        mock_axis.length.return_value = 1
        mock_axis.label = MagicMock(side_effect=["0"])
        mock_axis.getUnit().symbol().utf8.return_value = dummy_unit
        ws.getAxis.return_value = mock_axis

        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.ToolTipRole)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_TOOLTIP_STRING_FOR_NUMERIC_AXIS.format(mock_section, 0, dummy_unit)
        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_tooltip_for_numeric_axis_with_binned_data(self):
        dummy_unit = "unit"
        ws = MockWorkspace()
        mock_axis = Mock()
        mock_axis.isNumeric.return_value = True
        ws.getNumberHistograms.return_value = 1
        mock_axis.length.return_value = 2
        mock_axis.label = MagicMock(side_effect=["0.5"])
        mock_axis.getUnit().symbol().utf8.return_value = dummy_unit
        ws.getAxis.return_value = mock_axis

        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.ToolTipRole)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_TOOLTIP_STRING_FOR_NUMERIC_AXIS.format(
            mock_section, 0.5, dummy_unit
        )
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_display_role_for_X_values(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Horizontal, Qt.DisplayRole)
        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_DISPLAY_STRING_FOR_X_VALUES.format(mock_section)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_tooltip_role_for_X_values(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Horizontal, Qt.ToolTipRole)

        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_TOOLTIP_STRING_FOR_X_VALUES.format(mock_section)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_display_role_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = (mock_return_values[mock_section] + mock_return_values[mock_section + 1]) / 2.0
        is_histogram_data = True

        self._run_test_headerData_horizontal_header_display_role(is_histogram_data, mock_return_values, mock_section, expected_bin_centre)

    def test_headerData_horizontal_header_display_role_not_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = mock_return_values[mock_section]
        is_histogram_data = False

        self._run_test_headerData_horizontal_header_display_role(is_histogram_data, mock_return_values, mock_section, expected_bin_centre)

    def _run_test_headerData_horizontal_header_display_role(self, is_histogram_data, mock_return_values, mock_section, expected_bin_centre):
        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.DisplayRole)

        ws.isHistogramData.assert_called_once_with()
        ws.readX.assert_called_once_with(0)
        ws.getAxis.assert_called_once_with(AXIS_INDEX_FOR_HORIZONTAL)
        ws.mock_axis.getUnit.assert_called_once_with()
        ws.mock_axis.mock_unit.symbol.assert_called_once_with()
        ws.mock_axis.mock_unit.mock_symbol.utf8.assert_called_once_with()
        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_DISPLAY_STRING.format(
            mock_section, expected_bin_centre, MockMantidSymbol.TEST_UTF8
        )
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_tooltip_role_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = (mock_return_values[mock_section] + mock_return_values[mock_section + 1]) / 2.0
        is_histogram_data = True

        self._run_test_headerData_horizontal_header_tooltip_role(is_histogram_data, mock_return_values, mock_section, expected_bin_centre)

    def test_headerData_horizontal_header_tooltip_role_not_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = mock_return_values[mock_section]
        is_histogram_data = False
        self._run_test_headerData_horizontal_header_tooltip_role(is_histogram_data, mock_return_values, mock_section, expected_bin_centre)

    def _run_test_headerData_horizontal_header_tooltip_role(self, is_histogram_data, mock_return_values, mock_section, expected_bin_centre):
        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.ToolTipRole)

        ws.isHistogramData.assert_called_once_with()
        ws.readX.assert_called_once_with(0)
        ws.getAxis.assert_called_once_with(AXIS_INDEX_FOR_HORIZONTAL)
        ws.mock_axis.getUnit.assert_called_once_with()
        ws.mock_axis.mock_unit.symbol.assert_called_once_with()
        ws.mock_axis.mock_unit.caption.assert_called_once_with()
        ws.mock_axis.mock_unit.mock_symbol.utf8.assert_called_once_with()

        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_TOOLTIP_STRING.format(
            mock_section, MockMantidUnit.TEST_CAPTION, expected_bin_centre, MockMantidSymbol.TEST_UTF8
        )
        self.assertEqual(expected_output, output)

    def test_not_common_bins_horizontal_display_role(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        is_histogram_data = False

        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        ws.isCommonBins = Mock(return_value=False)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.DisplayRole)

        self.assertEqual(MatrixWorkspaceTableViewModel.HORIZONTAL_BINS_VARY_DISPLAY_STRING.format(mock_section), output)

    def test_not_common_bins_horizontal_tooltip_role(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        is_histogram_data = False

        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        ws.isCommonBins = Mock(return_value=False)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.ToolTipRole)

        self.assertEqual(MatrixWorkspaceTableViewModel.HORIZONTAL_BINS_VARY_TOOLTIP_STRING.format(mock_section), output)

    def test_histogram_data_has_one_extra_x_column(self):
        """
        Test that an extra column is added if the workspace is HistogramData. This is the column that
        contains the right boundary for the last bin.
        """
        mock_data = [1, 2, 3, 4, 5, 6, 7]
        data_len = len(mock_data)
        ws = MockWorkspace(read_return=mock_data, isHistogramData=True)

        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)

        self.assertEqual(data_len + 1, model.columnCount())

        # test that it is not added to Y and E
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)

        self.assertEqual(data_len, model.columnCount())

        model_type = MatrixWorkspaceTableViewModelType.e
        model = MatrixWorkspaceTableViewModel(ws, model_type)

        self.assertEqual(data_len, model.columnCount())

    def test_set_unicode_unit_label(self):
        """
        Set the label of the x-axis using ascii only with a non-ascii character and make sure it's handled properly.
        """
        ws = CreateWorkspace(DataX=[0, 1, 2], DataY=[3, 7, 5], DataE=[0.2, 0.3, 0.1], NSpec=1)
        label_unit = ws.getAxis(0).setUnit("Label")
        microseconds = "\u00b5s"
        # Second argument will implicitly call the ascii only constructor of UnitLabel.
        # We are intentionally passing a non-ascii string to try and break it.
        label_unit.setLabel("Time", microseconds)

        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        header = model.headerData(0, Qt.Horizontal, Qt.DisplayRole)

        # Header should contain the microseconds unicode string.
        self.assertTrue(microseconds in header)


if __name__ == "__main__":
    unittest.main()
