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

from mock import Mock

from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModel, \
    MatrixWorkspaceTableViewModelType

AXIS_INDEX_FOR_HORIZONTAL = 0
AXIS_INDEX_FOR_VERTICAL = 1


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


class MockMantidSymbol:
    TEST_ASCII = "MANTID_ASCII_SYMBOL"
    TEST_UTF8 = "MANTID_UTF8_SYMBOL"

    def __init__(self):
        self.utf8 = Mock(return_value=self.TEST_UTF8)
        self.ascii = Mock(return_value=self.TEST_ASCII)


class MockMantidUnit:
    TEST_CAPTION = "MANTID_TEST_CAPTION"

    def __init__(self):
        self.mock_symbol = MockMantidSymbol()
        self.symbol = Mock(return_value=self.mock_symbol)
        self.caption = Mock(return_value=self.TEST_CAPTION)


class MockMantidAxis:
    TEST_LABEL = "MANTID_TEST_AXIS"

    def __init__(self):
        self.label = Mock(return_value=self.TEST_LABEL)

        self.mock_unit = MockMantidUnit()
        self.getUnit = Mock(return_value=self.mock_unit)


class MockQModelIndexSibling:
    TEST_SIBLING_DATA = "MANTID_TEST_SIBLING_DATA"

    def __init__(self):
        self.data = Mock(return_value=self.TEST_SIBLING_DATA)


class MockQModelIndex:

    def __init__(self, row, column):
        self.row = Mock(return_value=row)
        self.column = Mock(return_value=column)
        self.mock_sibling = MockQModelIndexSibling()
        self.sibling = Mock(return_value=self.mock_sibling)


class MockSpectrumInfo:
    def __init__(self):
        self.hasDetectors = None
        self.isMasked = None
        self.isMonitor = None


class MockSpectrum:
    TEST_SPECTRUM_NO = 123123

    def __init__(self):
        self.getSpectrumNo = Mock(return_value=self.TEST_SPECTRUM_NO)


class MockWorkspace:
    TEST_NAME = "THISISAtestWORKSPACE"

    @staticmethod
    def _return_MockSpectrumInfo():
        return MockSpectrumInfo()

    def __init__(self, read_return=None, axes=2, isHistogramData=True):
        if read_return is None:
            read_return = [1, 2, 3, 4, 5]
        # This is assigned to a function, as the original implementation is a function that returns
        # the spectrumInfo object
        self.spectrumInfo = self._return_MockSpectrumInfo
        self.getNumberHistograms = Mock(return_value=1)
        self.isHistogramData = Mock(return_value=isHistogramData)
        self.blocksize = Mock(return_value=len(read_return))
        self.readX = Mock(return_value=read_return)
        self.readY = Mock(return_value=read_return)
        self.readE = Mock(return_value=read_return)
        self.axes = Mock(return_value=axes)
        self.hasMaskedBins = None
        self.maskedBinsIndices = None
        self.isCommonBins = Mock(return_value=True)

        self.mock_spectrum = MockSpectrum()
        self.getSpectrum = Mock(return_value=self.mock_spectrum)

        self.mock_axis = MockMantidAxis()
        self.getAxis = Mock(return_value=self.mock_axis)

        self.name = None
