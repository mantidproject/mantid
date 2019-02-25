# coding=utf-8
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

from qtpy import QtGui
from qtpy.QtCore import QVariant, Qt, QAbstractTableModel
from mantid.py3compat import Enum


class MatrixWorkspaceTableViewModelType(Enum):
    x = 'x'
    y = 'y'
    e = 'e'


class MatrixWorkspaceTableViewModel(QAbstractTableModel):
    HORIZONTAL_HEADER_DISPLAY_STRING = u"{0}\n{1:0.1f}{2}"
    HORIZONTAL_HEADER_TOOLTIP_STRING = u"index {0}\n{1} {2:0.1f}{3} (bin centre)"

    HORIZONTAL_HEADER_DISPLAY_STRING_FOR_X_VALUES = "{0}"
    HORIZONTAL_HEADER_TOOLTIP_STRING_FOR_X_VALUES = "index {0}"

    VERTICAL_HEADER_DISPLAY_STRING = "{0} {1}"
    VERTICAL_HEADER_TOOLTIP_STRING = "index {0}\nspectra no {1}"

    HORIZONTAL_BINS_VARY_DISPLAY_STRING = "{0}\nbins vary"
    HORIZONTAL_BINS_VARY_TOOLTIP_STRING = "index {0}\nbin centre value varies\nRebin to set common bins"

    MASKED_MONITOR_ROW_STRING = "This is a masked monitor spectrum. "
    MASKED_ROW_STRING = "This is a masked spectrum. "

    MONITOR_ROW_STRING = "This is a monitor spectrum. "
    MASKED_BIN_STRING = "This bin is masked. "

    def __init__(self, ws, model_type):
        """
        :param ws:
        :param model_type: MatrixWorkspaceTableViewModelType
        :type model_type: MatrixWorkspaceTableViewModelType
        """
        assert model_type in [MatrixWorkspaceTableViewModelType.x, MatrixWorkspaceTableViewModelType.y,
                              MatrixWorkspaceTableViewModelType.e], "The Model type must be either X, Y or E."

        super(MatrixWorkspaceTableViewModel, self).__init__()

        self.ws = ws
        self.ws_spectrum_info = self.ws.spectrumInfo()
        self.row_count = self.ws.getNumberHistograms()
        self.column_count = self.ws.blocksize()

        self.masked_rows_cache = []
        self.monitor_rows_cache = []
        self.masked_bins_cache = {}

        self.masked_color = QtGui.QColor(240, 240, 240)

        self.monitor_color = QtGui.QColor(255, 253, 209)

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
        else:
            raise ValueError("Unknown model type {0}".format(self.type))

    def _makeVerticalHeader(self, section, role):
        axis_index = 1
        # check that the vertical axis actually exists in the workspace
        if self.ws.axes() > axis_index:
            if role == Qt.DisplayRole:
                return self.VERTICAL_HEADER_DISPLAY_STRING.format(section, self.ws.getAxis(axis_index).label(section))
            else:
                spectrum_number = self.ws.getSpectrum(section).getSpectrumNo()
                return self.VERTICAL_HEADER_TOOLTIP_STRING.format(section, spectrum_number)
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
        if role == Qt.DisplayRole:
            # DisplayRole determines the text of each cell
            return str(self.relevant_data(row)[index.column()])
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
            elif self.checkMaskedBinCache(row, index):
                return self.masked_color

        elif role == Qt.ToolTipRole:
            tooltip = QVariant()
            if self.checkMaskedCache(row):
                if self.checkMonitorCache(row):
                    tooltip = self.MASKED_MONITOR_ROW_STRING
                else:
                    tooltip = self.MASKED_ROW_STRING
            elif self.checkMonitorCache(row):
                tooltip = self.MONITOR_ROW_STRING
                if self.checkMaskedBinCache(row, index):
                    tooltip += self.MASKED_BIN_STRING
            elif self.checkMaskedBinCache(row, index):
                tooltip = self.MASKED_BIN_STRING
            return tooltip
        else:
            return QVariant()

    def checkMaskedCache(self, row):
        if row in self.masked_rows_cache:
            return True
        elif self.ws_spectrum_info.hasDetectors(row) and self.ws_spectrum_info.isMasked(row):
            self.masked_rows_cache.append(row)
            return True

    def checkMonitorCache(self, row):
        if row in self.monitor_rows_cache:
            return True
        elif self.ws_spectrum_info.hasDetectors(row) and self.ws_spectrum_info.isMonitor(row):
            self.monitor_rows_cache.append(row)
            return True

    def checkMaskedBinCache(self, row, index):
        if row in self.masked_bins_cache:
            # retrieve the masked bins IDs from the cache
            if index.column() in self.masked_bins_cache[row]:
                return True

        elif self.ws.hasMaskedBins(row):
            masked_bins = self.ws.maskedBinsIndices(row)
            if index.column() in masked_bins:
                self.masked_bins_cache[row] = masked_bins
                return True
