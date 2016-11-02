#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.kernel import Direction, Stats
import mantid.simpleapi as ms
from mantid import mtd, logger
import numpy as np
from six import iteritems


def _stats_to_dict(stats):
    """
    Converts a Statstics object to a dictionary.
    @param stats Statistics object to convertToWaterfall
    @return Dictionary of statistics
    """
    stat_dict = dict()
    stat_dict['standard_deviation'] = stats.standard_deviation
    stat_dict['maximum'] = stats.maximum
    stat_dict['minimum'] = stats.minimum
    stat_dict['mean'] = stats.mean
    stat_dict['median'] = stats.median
    return stat_dict


class StatisticsOfTableWorkspace(PythonAlgorithm):

    def category(self):
        return 'Utility\\Workspaces'

    def summary(self):
        return 'Calcuates columns statistics of a table workspace.'

    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty('InputWorkspace', '', Direction.Input),
                             doc='Input table workspace.')
        self.declareProperty(ITableWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Output workspace contatining column statitics.')

    def PyExec(self):
        in_ws = mtd[self.getPropertyValue('InputWorkspace')]
        out_ws_name = self.getPropertyValue('OutputWorkspace')

        out_ws = ms.CreateEmptyTableWorkspace(OutputWorkspace=out_ws_name)

        out_ws.addColumn('str', 'statistic')

        stats = {
            'standard_deviation': dict(),
            'maximum': dict(),
            'minimum': dict(),
            'mean': dict(),
            'median': dict(),
        }

        for name in in_ws.getColumnNames():
            try:
                col_stats = _stats_to_dict(Stats.getStatistics(np.array([float(v) for v in in_ws.column(name)])))
                for statname in stats:
                    stats[statname][name] = col_stats[statname]
                out_ws.addColumn('float', name)
            except ValueError:
                logger.notice('Column \'%s\' is not numerical, skipping' % name)

        for name, stat in iteritems(stats):
            stat1 = dict(stat)
            stat1['statistic'] = name
            out_ws.addRow(stat1)

        self.setProperty('OutputWorkspace', out_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(StatisticsOfTableWorkspace)
