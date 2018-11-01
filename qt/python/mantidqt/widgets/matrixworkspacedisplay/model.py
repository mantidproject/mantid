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

from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModel, \
    MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayModel(object):
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    def __init__(self, ws):
        self._ws = ws

    def get_name(self):
        return self._ws.name()

    def get_spectrum_label(self, index):
        """
        :type index: int
        :param index: The index for which the label will be retrieved
        :return:
        """
        return self._ws.getAxis(1).label(index)

    def get_spectrum_plot_label(self, index):
        """
        :type index: int
        :param index: The index for which the plot label will be constructed
        :return:
        """
        return self.SPECTRUM_PLOT_LEGEND_STRING.format(self.get_name(), self.get_spectrum_label(index))

    def get_bin_label(self, index):
        """
        :type index: int
        :param index: The index for which the label will be retrieved
        :return:
        """
        return self._ws.getAxis(0).label(index)

    def get_bin_plot_label(self, index):
        """
        :type index: int
        :param index: The index for which the plot label will be constructed
        :return:
        """
        return self.BIN_PLOT_LEGEND_STRING.format(self.get_name(), self.get_bin_label(index))

    def get_item_model(self):
        return MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.x), \
               MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.y), \
               MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.e)
