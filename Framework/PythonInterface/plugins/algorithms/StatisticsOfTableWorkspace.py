# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)

import collections

import numpy as np
from six import iteritems

import mantid.simpleapi as ms
from mantid import logger, mtd
from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, IntArrayBoundedValidator, IntArrayProperty, Stats


def _stats_to_dict(stats):
    """
    Converts a Statistics object to an ordered dictionary.
    @param stats Statistics object to convertToWaterfall
    @return Dictionary of statistics
    """
    stat_dict = collections.OrderedDict()
    stat_dict['StandardDev'] = stats.standard_deviation
    stat_dict['Maximum'] = stats.maximum
    stat_dict['Minimum'] = stats.minimum
    stat_dict['Mean'] = stats.mean
    stat_dict['Median'] = stats.median
    return stat_dict


class StatisticsOfTableWorkspace(PythonAlgorithm):

    def category(self):
        return 'Utility\\Workspaces'

    def summary(self):
        return 'Calcuates columns statistics of a table workspace.'

    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty('InputWorkspace', '', Direction.Input),
                             doc='Input table workspace.')
        validator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(
            IntArrayProperty('ColumnIndices', values=[], direction=Direction.Input, validator=validator),
            'Comma separated list of column indices for which statistics will be separated')
        self.declareProperty(ITableWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Output workspace containing column statistics.')

    def PyExec(self):
        in_ws = mtd[self.getPropertyValue('InputWorkspace')]
        indices_list = self.getPropertyValue('ColumnIndices')
        out_ws_name = self.getPropertyValue('OutputWorkspace')
        column_names = in_ws.getColumnNames()

        # If column indices are not provided, then default to _ALL_ columns
        if len(indices_list) > 0:
            indices_list = [int(x) for x in indices_list.split(',')]
        else:
            indices_list = range(len(column_names))

        out_ws = ms.CreateEmptyTableWorkspace(OutputWorkspace=out_ws_name)

        out_ws.addColumn('str', 'Statistic')

        stats = collections.OrderedDict([
            ('StandardDev', collections.OrderedDict()),
            ('Minimum', collections.OrderedDict()),
            ('Median', collections.OrderedDict()),
            ('Maximum', collections.OrderedDict()),
            ('Mean', collections.OrderedDict()),
        ])

        for index in indices_list:
            column_name = column_names[index]
            try:
                column_data = np.array([float(v) for v in in_ws.column(index)])
                col_stats = _stats_to_dict(Stats.getStatistics(column_data))
                for stat_name in stats:
                    stats[stat_name][column_name] = col_stats[stat_name]
                out_ws.addColumn('float', column_name)
            except RuntimeError:
                logger.notice('Column \'%s\' is not numerical, skipping' % column_name)
            except:
                logger.notice('Column \'%s\' is not numerical, skipping' % column_name)

        for index, stat_name in iteritems(stats):
            stat = collections.OrderedDict(stat_name)
            stat['Statistic'] = index
            out_ws.addRow(stat)

        self.setProperty('OutputWorkspace', out_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(StatisticsOfTableWorkspace)
