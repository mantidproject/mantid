# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

from mantid.py3compat.mock import Mock
from mantidqt.utils.testing.strict_mock import StrictMock

AXIS_INDEX_FOR_HORIZONTAL = 0
AXIS_INDEX_FOR_VERTICAL = 1


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


class MockSpectrumInfo:
    def __init__(self):
        self.hasDetectors = None
        self.isMasked = None
        self.isMonitor = None


class MockSpectrum:
    SPECTRUM_NO = 123123

    def __init__(self):
        self.getSpectrumNo = Mock(return_value=self.SPECTRUM_NO)


class MockWorkspace:
    TEST_NAME = "THISISAtestWORKSPACE"

    NUMBER_OF_ROWS_AND_COLUMNS = 5
    COLS = NUMBER_OF_ROWS_AND_COLUMNS
    ROWS = COLS

    @staticmethod
    def _return_MockSpectrumInfo():
        return MockSpectrumInfo()

    def __init__(self, read_return=None, axes=2, isHistogramData=True):
        if read_return is None:
            read_return = [1, 2, 3, 4, 5]
        # This is assigned to a function, as the original implementation is a function that returns
        # the spectrumInfo object
        self.spectrumInfo = self._return_MockSpectrumInfo
        self.getNumberHistograms = StrictMock(return_value=1)
        self.isHistogramData = StrictMock(return_value=isHistogramData)
        self.blocksize = StrictMock(return_value=len(read_return))
        self.readX = StrictMock(return_value=read_return)
        self.readY = StrictMock(return_value=read_return)
        self.readE = StrictMock(return_value=read_return)
        self.axes = StrictMock(return_value=axes)
        self.hasMaskedBins = None
        self.maskedBinsIndices = None
        self.isCommonBins = StrictMock(return_value=True)

        self.column_types = ["int", "float", "string", "v3d", "bool"]
        self.columnTypes = StrictMock(return_value=self.column_types)

        self.mock_spectrum = MockSpectrum()
        self.getSpectrum = StrictMock(return_value=self.mock_spectrum)

        self.mock_axis = MockMantidAxis()
        self.getAxis = StrictMock(return_value=self.mock_axis)

        self.setCell = StrictMock()

        self.name = StrictMock(return_value=self.TEST_NAME)

        self._column_names = []
        for i in range(self.COLS):
            self._column_names.append("col{0}".format(i))

        self.getColumnNames = StrictMock(return_value=self._column_names)
        self.column_count = self.COLS
        self.columnCount = StrictMock(return_value=self.column_count)

        self.row_count = self.ROWS
        self.rowCount = StrictMock(return_value=self.row_count)

        self.column = StrictMock(return_value=[1] * self.row_count)

        self.emit_repaint = StrictMock()

        self.getPlotType = StrictMock()
