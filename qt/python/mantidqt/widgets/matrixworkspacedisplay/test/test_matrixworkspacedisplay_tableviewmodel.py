# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

import qtpy
from qtpy import QtCore

from mantid.simpleapi import Load
from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModel, \
    MatrixWorkspaceTableViewModelType

import unittest
from mock import Mock, call
from qtpy.QtCore import Qt


class MockMantidSymbol:
    TEST_ASCII = "MANTID_ASCII_SYMBOL"
    TEST_UTF8 = "MANTID_UTF8_SYMBOL"

    def __init__(self):
        self.utf8 = Mock(return_value=self.TEST_UTF8)
        self.ascii = Mock(return_value=self.TEST_ASCII)


class MockMantidUnit:
    def __init__(self):
        self.mock_symbol = MockMantidSymbol()
        self.symbol = Mock(return_value=self.mock_symbol)


class MockMantidAxis:
    TEST_LABEL = "MANTID_TEST_AXIS"

    def __init__(self):
        self.label = Mock(return_value=self.TEST_LABEL)

        self.mock_unit = MockMantidUnit()
        self.getUnit = Mock(return_value=self.mock_unit)


class MockQModelIndex:
    def __init__(self, row, column):
        self.row = Mock(return_value=row)
        self.column = Mock(return_value=column)


class MockSpectrumInfo:
    def __init__(self):
        self.hasDetectors = Mock()
        self.isMasked = Mock()
        self.isMonitor = Mock()


class MockSpectrum:
    TEST_SPECTRUM_NO = 123123

    def __init__(self):
        self.getSpectrumNo = Mock(return_value=self.TEST_SPECTRUM_NO)


class MockWorkspace:
    @staticmethod
    def _return_MockSpectrumInfo():
        return MockSpectrumInfo()

    def __init__(self, read_return=None, axes=2, isHistogramData=True):
        if read_return is None:
            read_return = [1, 2, 3, 4, 5]
        # This is assigned to a function, as the original implementation is a function that returns
        # the spectrumInfo object
        self.spectrumInfo = self._return_MockSpectrumInfo
        self.getNumberHistograms = Mock()
        self.isHistogramData = Mock(return_value=isHistogramData)
        self.blocksize = Mock()
        self.readX = Mock(return_value=read_return)
        self.readY = Mock(return_value=read_return)
        self.readE = Mock(return_value=read_return)
        self.axes = Mock(return_value=axes)
        self.hasMaskedBins = Mock()
        self.maskedBinsIndices = Mock()

        self.mock_spectrum = MockSpectrum()
        self.getSpectrum = Mock(return_value=self.mock_spectrum)

        self.mock_axis = MockMantidAxis()
        self.getAxis = Mock(return_value=self.mock_axis)


class MatrixWorkspaceDisplayTableViewModelTest(unittest.TestCase):
    WORKSPACE = r"C:\Users\qbr77747\dev\m\workbench_matrixworkspace\test_masked_bins.nxs"
    AXIS_INDEX_FOR_HORIZONTAL = 0
    AXIS_INDEX_FOR_VERTICAL = 1

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
        self.assertEqual(ws.readX, model.relevant_data)
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.y)
        self.assertEqual(ws.readY, model.relevant_data)
        model = MatrixWorkspaceTableViewModel(ws, MatrixWorkspaceTableViewModelType.e)
        self.assertEqual(ws.readE, model.relevant_data)

    def test_invalid_model_type(self):
        ws = MockWorkspace()
        with self.assertRaises(AssertionError) as cm:
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
        model.relevant_data.assert_called_once_with(row)
        self.assertEqual(str(mock_data[column]), output)

    def test_data_background_role_masked_row(self):
        ws, model, row, index = self._common_for_test_data_background()

        model.ws_spectrum_info.isMasked = Mock(return_value=True)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_once_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)

        index.row.assert_called_once_with()
        self.assertFalse(index.column.called)

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
        self.assertFalse(index.column.called)

    def test_data_background_role_monitor_row(self):
        ws, model, row, index = self._common_for_test_data_background()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=True)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)

        index.row.assert_called_once_with()
        self.assertFalse(index.column.called)

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
        self.assertFalse(index.column.called)

    def test_data_background_role_masked_bin(self):
        ws, model, row, index = self._common_for_test_data_background()

        model.ws_spectrum_info.isMasked = Mock(return_value=False)
        model.ws_spectrum_info.isMonitor = Mock(return_value=False)

        output = model.data(index, Qt.BackgroundRole)

        model.ws_spectrum_info.hasDetectors.assert_called_with(row)
        model.ws_spectrum_info.isMasked.assert_called_once_with(row)
        model.ws_spectrum_info.isMonitor.assert_called_once_with(row)
        ws.hasMaskedBins.assert_called_once_with(row)
        ws.maskedBinsIndices.assert_called_once_with(row)

        index.row.assert_called_once_with()
        index.column.assert_called_once_with()

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

    def _common_for_test_data_background(self):
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

    def test_row_and_column_count(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        MatrixWorkspaceTableViewModel(ws, model_type)
        # these are called when the TableViewModel is initialised
        ws.getNumberHistograms.assert_called_once_with()
        ws.blocksize.assert_called_once_with()

    def test_headerData_not_display_or_tooltip(self):
        if not qtpy.PYQT5:
            self.skipTest("QVariant cannot be instantiated in QT4, and the test fails with an error.")
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

        ws.getAxis.assert_called_once_with(self.AXIS_INDEX_FOR_VERTICAL)
        ws.mock_axis.label.assert_called_once_with(mock_section)

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_DISPLAY_STRING.format(mock_section, " ",
                                                                                              MockMantidAxis.TEST_LABEL)

        self.assertEqual(expected_output, output)

    def test_headerData_vertical_header_tooltip_role(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Vertical, Qt.ToolTipRole)

        ws.getSpectrum.assert_called_once_with(mock_section)
        ws.mock_spectrum.getSpectrumNo.assert_called_once_with()

        expected_output = MatrixWorkspaceTableViewModel.VERTICAL_HEADER_TOOLTIP_STRING.format(mock_section,
                                                                                              MockSpectrum.TEST_SPECTRUM_NO)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_display_role_for_X_values(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Horizontal, Qt.DisplayRole)
        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_DISPLAY_STRING_FOR_X_VALUES.format(
            mock_section)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_tooltip_role_for_X_values(self):
        ws = MockWorkspace()
        model_type = MatrixWorkspaceTableViewModelType.x
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        mock_section = 0
        output = model.headerData(mock_section, Qt.Horizontal, Qt.ToolTipRole)

        expected_output = MatrixWorkspaceTableViewModel.HORIZONTAL_HEADER_TOOLTIP_STRING_FOR_X_VALUES.format(
            mock_section)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_display_role_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = (mock_return_values[mock_section] + mock_return_values[mock_section + 1]) / 2.0
        is_histogram_data = True

        self._run_test_headerData_horizontal_header_display_role(is_histogram_data, mock_return_values, mock_section,
                                                                 expected_bin_centre)

    def test_headerData_horizontal_header_display_role_not_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = mock_return_values[mock_section]
        is_histogram_data = False

        self._run_test_headerData_horizontal_header_display_role(is_histogram_data, mock_return_values, mock_section,
                                                                 expected_bin_centre)

    def _run_test_headerData_horizontal_header_display_role(self, is_histogram_data, mock_return_values, mock_section,
                                                            expected_bin_centre):
        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.DisplayRole)

        ws.isHistogramData.assert_called_once_with()
        ws.readX.assert_called_once_with(0)
        ws.getAxis.assert_called_once_with(self.AXIS_INDEX_FOR_HORIZONTAL)
        ws.mock_axis.getUnit.assert_called_once_with()
        ws.mock_axis.mock_unit.symbol.assert_called_once_with()
        ws.mock_axis.mock_unit.mock_symbol.utf8.assert_called_once_with()
        expected_output = MatrixWorkspaceTableViewModel \
            .HORIZONTAL_HEADER_DISPLAY_STRING \
            .format(mock_section, expected_bin_centre, MockMantidSymbol.TEST_UTF8)
        self.assertEqual(expected_output, output)

    def test_headerData_horizontal_header_tooltip_role_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = (mock_return_values[mock_section] + mock_return_values[mock_section + 1]) / 2.0
        is_histogram_data = True

        self._run_test_headerData_horizontal_header_tooltip_role(is_histogram_data, mock_return_values, mock_section,
                                                                 expected_bin_centre)

    def test_headerData_horizontal_header_tooltip_role_not_histogram_data(self):
        mock_section = 0
        mock_return_values = [0, 1, 2, 3, 4, 5, 6]
        expected_bin_centre = mock_return_values[mock_section]
        is_histogram_data = False
        self._run_test_headerData_horizontal_header_tooltip_role(is_histogram_data, mock_return_values, mock_section,
                                                                 expected_bin_centre)

    def _run_test_headerData_horizontal_header_tooltip_role(self, is_histogram_data, mock_return_values, mock_section,
                                                            expected_bin_centre):
        ws = MockWorkspace(read_return=mock_return_values, isHistogramData=is_histogram_data)
        model_type = MatrixWorkspaceTableViewModelType.y
        model = MatrixWorkspaceTableViewModel(ws, model_type)
        output = model.headerData(mock_section, Qt.Horizontal, Qt.ToolTipRole)

        ws.isHistogramData.assert_called_once_with()
        ws.readX.assert_called_once_with(0)
        ws.getAxis.assert_called_once_with(self.AXIS_INDEX_FOR_HORIZONTAL)
        ws.mock_axis.getUnit.assert_called_once_with()
        ws.mock_axis.mock_unit.symbol.assert_called_once_with()
        ws.mock_axis.mock_unit.mock_symbol.utf8.assert_called_once_with()
        ws.mock_axis.mock_unit.mock_symbol.ascii.assert_called_once_with()

        expected_output = MatrixWorkspaceTableViewModel \
            .HORIZONTAL_HEADER_TOOLTIP_STRING \
            .format(mock_section, MockMantidSymbol.TEST_ASCII, expected_bin_centre, MockMantidSymbol.TEST_UTF8)
        self.assertEqual(expected_output, output)


if __name__ == '__main__':
    unittest.main()
