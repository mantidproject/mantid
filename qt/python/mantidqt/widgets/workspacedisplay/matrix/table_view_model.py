# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantid workbench.
#
#
from qtpy import QtGui
from qtpy.QtCore import QVariant, Qt, QAbstractTableModel
from enum import Enum


class MatrixWorkspaceTableViewModelType(Enum):
    x = 'x'
    y = 'y'
    e = 'e'
    dx = 'dx'


class MatrixWorkspaceTableViewModel(QAbstractTableModel):
    HORIZONTAL_HEADER_DISPLAY_STRING = u"{0}\n{1:0.4f}{2}"
    HORIZONTAL_HEADER_TOOLTIP_STRING = u"index {0}\n{1} {2:0.6f}{3} (bin centre)"

    HORIZONTAL_HEADER_DISPLAY_STRING_FOR_X_VALUES = "{0}"
    HORIZONTAL_HEADER_TOOLTIP_STRING_FOR_X_VALUES = "index {0}"

    VERTICAL_HEADER_DISPLAY_STRING = "{0} {1}"
    VERTICAL_HEADER_TOOLTIP_STRING = "index {0}\nspectra no {1}"

    VERTICAL_HEADER_DISPLAY_STRING_FOR_NUMERIC_AXIS = u"{0} {1:0.2f}{2}"
    VERTICAL_HEADER_TOOLTIP_STRING_FOR_NUMERIC_AXIS = u"index {0}\nbin center {1:0.2f}{2}"

    HORIZONTAL_BINS_VARY_DISPLAY_STRING = "{0}\nbins vary"
    HORIZONTAL_BINS_VARY_TOOLTIP_STRING = "index {0}\nbin centre value varies\nRebin to set common bins"

    BLANK_CELL_STRING = ""

    MASKED_MONITOR_ROW_TOOLTIP = "This is a masked monitor spectrum. "
    MASKED_ROW_TOOLTIP = "This is a masked spectrum. "

    MONITOR_ROW_TOOLTIP = "This is a monitor spectrum. "
    MASKED_BIN_TOOLTIP = "This bin is masked. "

    BLANK_CELL_TOOLTIP = "This cell is blank because the workspace is ragged."

    def __init__(self, ws, model_type):
        """
        :param ws:
        :param model_type: MatrixWorkspaceTableViewModelType
        :type model_type: MatrixWorkspaceTableViewModelType
        """
        assert model_type in [MatrixWorkspaceTableViewModelType.x, MatrixWorkspaceTableViewModelType.y,
                              MatrixWorkspaceTableViewModelType.e, MatrixWorkspaceTableViewModelType.dx], \
            "The Model type must be either X, Y, E or DX."

        super(MatrixWorkspaceTableViewModel, self).__init__()

        self.ws = ws
        self.ws_spectrum_info = self.ws.spectrumInfo()
        self.row_count = self.ws.getNumberHistograms()
        self.column_count = self.ws.getMaxNumberBins()

        self.masked_rows_cache = []
        self.monitor_rows_cache = []
        self.masked_bins_cache = {}
        self.blank_cell_cache = {}

        self.masked_color = QtGui.QColor(240, 240, 240)
        self.monitor_color = QtGui.QColor(255, 253, 209)
        self.blank_cell_color = QtGui.QColor(145, 139, 141)

        self.type = model_type
        if self.type == MatrixWorkspaceTableViewModelType.x:
            self.relevant_data = self.ws.readX

            # add another column if the workspace is histogram data
            # this will contain the right boundary for the last bin
            if self.ws.isHistogramData():
                self.column_count += 1

        elif self.type == MatrixWorkspaceTableViewModelType.y:
            self.relevant_data = self.ws.readY
        elif self.type == MatrixWorkspaceTableViewModelType.e:
            self.relevant_data = self.ws.readE
        elif self.type == MatrixWorkspaceTableViewModelType.dx:
            self.relevant_data = self.ws.readDx
        else:
            raise ValueError("Unknown model type {0}".format(self.type))

    def _makeVerticalHeader(self, section, role):
        def _numeric_axis_value_unit(axis):
            # binned/point data
            if axis.length() == self.ws.getNumberHistograms() + 1:
                value = 0.5 * (float(axis.label(section)) + float(axis.label(section + 1)))
            else:
                value = float(axis.label(section))
            return value, axis.getUnit().symbol().utf8()

        axis_index = 1
        # check that the vertical axis actually exists in the workspace
        if self.ws.axes() > axis_index:
            axis = self.ws.getAxis(axis_index)
            if role == Qt.DisplayRole:
                if not axis.isNumeric():
                    return self.VERTICAL_HEADER_DISPLAY_STRING.format(
                        section, axis.label(section))
                else:
                    display_value, unit = _numeric_axis_value_unit(axis)
                    return self.VERTICAL_HEADER_DISPLAY_STRING_FOR_NUMERIC_AXIS.format(section, display_value, unit)
            else:
                if not axis.isNumeric():
                    spectrum_number = self.ws.getSpectrum(section).getSpectrumNo()
                    return self.VERTICAL_HEADER_TOOLTIP_STRING.format(section, spectrum_number)
                else:
                    display_value, unit = _numeric_axis_value_unit(axis)
                    return self.VERTICAL_HEADER_TOOLTIP_STRING_FOR_NUMERIC_AXIS.format(section, display_value, unit)
        else:
            raise NotImplementedError("What do we do here? Handle if the vertical axis does NOT exist")

    def _makeHorizontalHeader(self, section, role):
        """

        :param section: The workspace index or bin number
        :param role: Qt.DisplayRole - is the label for the header
                      or Qt.TooltipRole - is the tooltip for the header when moused over
        :return: The formatted header string
        """
        # X values get simpler labels
        if self.type == MatrixWorkspaceTableViewModelType.x:
            if role == Qt.DisplayRole:
                return self.HORIZONTAL_HEADER_DISPLAY_STRING_FOR_X_VALUES.format(section)
            else:
                # format for the tooltip
                return self.HORIZONTAL_HEADER_TOOLTIP_STRING_FOR_X_VALUES.format(section)

        if not self.ws.isCommonBins():
            if role == Qt.DisplayRole:
                return self.HORIZONTAL_BINS_VARY_DISPLAY_STRING.format(section)
            else:
                # format for the tooltip
                return self.HORIZONTAL_BINS_VARY_TOOLTIP_STRING.format(section)

        # for the Y and E values, create a label with the units
        axis_index = 0
        x_vec = self.ws.readX(0)
        if self.ws.isHistogramData():
            bin_centre_value = (x_vec[section] + x_vec[section + 1]) / 2.0
        else:
            bin_centre_value = x_vec[section]

        unit = self.ws.getAxis(axis_index).getUnit()
        if role == Qt.DisplayRole:
            return self.HORIZONTAL_HEADER_DISPLAY_STRING.format(section, bin_centre_value, unit.symbol().utf8())
        else:
            # format for the tooltip
            return self.HORIZONTAL_HEADER_TOOLTIP_STRING.format(section, unit.caption(), bin_centre_value,
                                                                unit.symbol().utf8())

    def headerData(self, section, orientation, role=None):
        if not (role == Qt.DisplayRole or role == Qt.ToolTipRole):
            return QVariant()

        if orientation == Qt.Vertical:
            return self._makeVerticalHeader(section, role)
        else:
            return self._makeHorizontalHeader(section, role)

    def rowCount(self, parent=None, *args, **kwargs):
        return self.row_count

    def columnCount(self, parent=None, *args, **kwargs):
        return self.column_count

    def data(self, index, role=None):
        row = index.row()
        column = index.column()
        if role == Qt.DisplayRole:
            # DisplayRole determines the text of each cell
            if self.has_data_at(row, column):
                return str(self.relevant_data(row)[column])
            # The cell is blank
            return self.BLANK_CELL_STRING
        elif role == Qt.BackgroundRole:
            # BackgroundRole determines the background of each cell

            # Checks if the row is MASKED, if so makes it the specified color for masked
            # The check for masked rows should be first as a monitor row can be masked as well - and we want it to be
            # colored as a masked row, rather than as a monitor row.
            # First do the check in the cache, and only if not present go through SpectrumInfo and cache it. This logic
            # is repeated in the other checks below
            if self.checkMaskedCache(row):
                return self.masked_color

            # Checks if the row is a MONITOR, if so makes it the specified color for monitors
            elif self.checkMonitorCache(row):
                return self.monitor_color

            # Checks if the BIN is MASKED, if so makes it the specified color for masked
            elif self.checkMaskedBinCache(row, column):
                return self.masked_color

            # Checks if the cell is BLANK, if so it returns the specified color for blank cells
            elif self.checkBlankCache(row, column):
                return self.blank_cell_color

        elif role == Qt.ToolTipRole:
            tooltip = QVariant()
            if self.checkMaskedCache(row):
                if self.checkMonitorCache(row):
                    tooltip = self.MASKED_MONITOR_ROW_TOOLTIP
                else:
                    tooltip = self.MASKED_ROW_TOOLTIP
            elif self.checkMonitorCache(row):
                tooltip = self.MONITOR_ROW_TOOLTIP
                if self.checkMaskedBinCache(row, column):
                    tooltip += self.MASKED_BIN_TOOLTIP
            elif self.checkMaskedBinCache(row, column):
                tooltip = self.MASKED_BIN_TOOLTIP
            elif self.checkBlankCache(row, column):
                tooltip = self.BLANK_CELL_TOOLTIP
            return tooltip
        else:
            return QVariant()

    def checkMaskedCache(self, row):
        """
        Checks to see if a spectrum is masked.
        :param row: The index of the spectrum in the workspace.
        :return: True if the spectrum is masked.
        """
        if row in self.masked_rows_cache:
            return True
        elif self.ws_spectrum_info.hasDetectors(row) and self.ws_spectrum_info.isMasked(row):
            self.masked_rows_cache.append(row)
            return True

    def checkMonitorCache(self, row):
        """
        Checks to see if a spectrum represents a monitor.
        :param row: The index of the spectrum in the workspace.
        :return: True if the spectrum is a monitor.
        """
        if row in self.monitor_rows_cache:
            return True
        elif self.ws_spectrum_info.hasDetectors(row) and self.ws_spectrum_info.isMonitor(row):
            self.monitor_rows_cache.append(row)
            return True

    def checkMaskedBinCache(self, row, column):
        """
        Checks to see if a specific cell is masked.
        :param row: The row index of the cell.
        :param column: The column index of the cell.
        :return: True if the cell is masked.
        """
        if row in self.masked_bins_cache:
            # retrieve the masked bins IDs from the cache
            if column in self.masked_bins_cache[row]:
                return True

        elif self.ws.hasMaskedBins(row):
            masked_bins = self.ws.maskedBinsIndices(row)
            if column in masked_bins:
                self.masked_bins_cache[row] = masked_bins
                return True

    def checkBlankCache(self, row, column):
        """
        Checks to see if a specific cell should be blank (because the workspace is ragged).
        :param row: The row index of the cell.
        :param column: The column index of the cell.
        :return: True if the cell should be blank.
        """
        if row in self.blank_cell_cache and column in self.blank_cell_cache[row]:
            return True
        elif not self.has_data_at(row, column):
            if row in self.blank_cell_cache:
                self.blank_cell_cache[row].append(column)
            else:
                self.blank_cell_cache[row] = [column]
            return True
        return False

    def has_data_at(self, row, column):
        """
        Checks to see if data exists at a specific location in the relevant_data.
        :param row: The row index of the data to check.
        :param column: The column index of the data to check.
        :return: True if data exists at a specific location.
        """
        try:
            row_data = self.relevant_data(row)
            return column < len(row_data)
        except IndexError:
            return False
