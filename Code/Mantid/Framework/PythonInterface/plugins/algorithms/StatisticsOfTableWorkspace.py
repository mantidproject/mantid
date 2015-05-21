from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.kernel import Direction, Stats
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid import mtd, logger
import numpy as np


def _stats_to_dict(s):
    """
    Converts a STatstics object to a dictionary.
    @param s Statistics object to convertToWaterfall
    @return Dictionary of statistics
    """
    d = dict()
    d['standard_deviation'] = s.standard_deviation
    d['maximum'] = s.maximum
    d['minimum'] = s.minimum
    d['mean'] = s.mean
    d['median'] = s.median
    return d


class StatisticsOfTableWorkspace(PythonAlgorithm):

    def category(self):
        return 'DataHandling'


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

        out_ws = CreateEmptyTableWorkspace(OutputWOrkspace=out_ws_name)

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
                s = _stats_to_dict(Stats.getStatistics(np.array([float(v) for v in in_ws.column(name)])))
                for statname in stats.keys():
                    stats[statname][name] = s[statname]
                out_ws.addColumn('float', name)
            except ValueError:
                logger.notice('Column \'%s\' is not numerical, skipping' % name)
                pass

        for name, s in stats.items():
            st = dict(s)
            st['statistic'] = name
            out_ws.addRow(st)

        self.setProperty('OutputWorkspace', out_ws_name)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(StatisticsOfTableWorkspace)
