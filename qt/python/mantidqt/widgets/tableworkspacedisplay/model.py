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

from mantid.dataobjects import PeaksWorkspace, TableWorkspace


class TableWorkspaceDisplayModel(object):
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    # PEAKS_WORKSPACE_EDITABLE_COLUMNS = ["RunNumber", "h", "k", "l"]

    def __init__(self, ws):
        if not isinstance(ws, TableWorkspace) and not isinstance(ws, PeaksWorkspace):
            raise ValueError("The workspace type is not supported: {0}".format(type(ws)))

        self.ws = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()

    @staticmethod
    def is_peaks_workspace(ws):
        return isinstance(ws, PeaksWorkspace)

    def get_name(self):
        return self.ws.name()

    def get_column_headers(self):
        return self.ws.getColumnNames()

    def get_column(self, index):
        return self.ws.column(index)

    def get_number_of_rows(self):
        return self.ws_num_rows

    def get_number_of_columns(self):
        return self.ws_num_cols

# class PeaksWorkspaceDisplayModel(TableWorkspaceDisplayModel):
#     def __init__(self, ws):
#         super(PeaksWorkspaceDisplayModel, self).__init__(ws)
#         self.sigma_col_index = None
#
#     def get_column_headers(self):
#         column_names = self.ws.getColumnNames()
#         self.sigma_col_index = column_names.index("SigInt") + 1
#         # insert the intensity/sigma after the sigma column
#         column_names.insert(self.sigma_col_index, "I/σ")
#         # update the number of columns
#         self.ws_num_cols = len(column_names)
#         return column_names
#
#     def get_column(self, index):
#         """
#         Get data for a column from the PeaksWorkspace.
#
#         Handles the index for the additional column Intensity/Sigma column correctly,
#         as the column itself is only added
#         :param index:
#         :return:
#         """
#         if index < self.sigma_col_index:
#             return self.ws.column(index)
#         elif index > self.sigma_col_index:
#             return self.ws.column(index - 1)
#         else:
#             num_rows = self.get_number_of_rows()
#             return [self.ws.getPeak(i).getIntensityOverSigma() for i in range(num_rows)]
