# pylint: disable=no-init
import stresstesting
import json
from mantid.simpleapi import *
from mantid.geometry import PointGroupFactory


class SortHKLTest(stresstesting.MantidStressTest):
    def runTest(self):
        self._ws = CreateSimulationWorkspace(Instrument='TOPAZ',
                                             BinParams='0,10000,20000',
                                             UnitX='TOF',
                                             OutputWorkspace='topaz_instrument_workspace')

        self._space_groups = ['Fm-3m', 'Im-3m']

        self._base_directory = 'SortHKL/'
        self._template_hkl = 'reflections_{0}.hkl'
        self._template_ub = 'ub_parameters_{0}.json'
        self._template_statistics = 'statistics_{0}.txt'

        self.test_SortHKLStatistics()

    def test_SortHKLStatistics(self):
        for space_group in self._space_groups:
            ub_parameters = self._load_ub_parameters(space_group)
            reflections = self._load_reflections(space_group, ub_parameters)
            statistics = self._calculate_statistics(reflections, space_group)
            reference_statistics = self._load_reference_statistics(space_group)

            self._compare_statistics(statistics, reference_statistics, space_group)

    def _load_ub_parameters(self, space_group):
        filename = FileFinder.Instance().getFullPath(self._base_directory + self._template_ub.format(space_group))

        ub_parameters = {}
        with open(filename, 'r') as ub_parameter_file:
            raw_ub_parameters = json.load(ub_parameter_file)

            # Mantid functions don't seem to like unicode so the dict is re-written
            ub_parameters.update(
                dict(
                    [(str(x), y if type(y) == float else str(y))
                     for x, y in raw_ub_parameters.iteritems()]
                ))

        return ub_parameters

    def _load_reflections(self, space_group, ub_parameters):
        filename = self._base_directory + self._template_hkl.format(space_group)

        hkls_from_file = LoadHKL(Filename=filename)

        actual_hkls = CreatePeaksWorkspace(InstrumentWorkspace=self._ws, NumberOfPeaks=0)

        for i in range(hkls_from_file.getNumberPeaks()):
            actual_hkls.addPeak(hkls_from_file.getPeak(i))

        SetUB(Workspace=actual_hkls, **ub_parameters)

        return actual_hkls

    def _calculate_statistics(self, reflections, space_group):
        point_group_name = PointGroupFactory.createPointGroup(space_group[1:]).getName()
        sorted, chi2, statistics = SortHKL(InputWorkspace=reflections,
                                           PointGroup=point_group_name)

        return statistics.row(0)

    def _load_reference_statistics(self, space_group):
        filename = FileFinder.Instance().getFullPath(
            self._base_directory + self._template_statistics.format(space_group))

        lines = []
        with open(filename, 'r') as statistics_file:
            for line in statistics_file:
                lines.append(line)

        keys = lines[0].split()
        values = [float(x) for x in lines[2].split()[2:]]

        return dict(zip(keys, values))

    def _compare_statistics(self, statistics, reference_statistics, space_group):
        self.assertEquals(round(statistics['Multiplicity'], 1), round(reference_statistics['<N>'], 1))

        print space_group
        print statistics
        print reference_statistics
