# pylint: disable=no-init
import stresstesting
import json
from mantid.simpleapi import *
from mantid.geometry import PointGroupFactory


class SortHKLTest(stresstesting.MantidStressTest):
    ''' System test for SortHKL

    This system test compares some of the output of SortHKL to statistics produced
    by running the program SORTAV [1] on the same data set.

    Since SORTAV processes HKL-files and those are small, the peaks are loaded from
    HKL-files and put into an empty PeaksWorkspace. Two additional files are read
    for the test, the parameters for SetUB in JSON-format and some of the output from
    the sortav.lp file which contains the output after a SORTAV-run.

    This system test is there to ensure the correctness what SortHKL does against
    the output of an established program.

    [1] SORTAV: ftp://ftp.hwi.buffalo.edu/pub/Blessing/Drear/sortav.use
        (and references therein).
    '''
    def runTest(self):
        self._ws = CreateSimulationWorkspace(Instrument='TOPAZ',
                                             BinParams='0,10000,20000',
                                             UnitX='TOF',
                                             OutputWorkspace='topaz_instrument_workspace')

        self._space_groups = ['Pm-3m', 'P4_mmm', 'Pmmm', 'Im-3m', 'Fm-3m', 'I4_mmm', 'Cmmm', 'Immm', 'Fmmm']

        self._centering_map = {'P': 'Primitive',
                               'I': 'Body centred',
                               'F': 'All-face centred',
                               'C': 'C-face centred'}

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
        point_group_name = PointGroupFactory.createPointGroup(space_group[1:].replace('_', '/')).getName()
        centering_name = self._centering_map[space_group[0]]
        sorted, chi2, statistics = SortHKL(InputWorkspace=reflections,
                                           PointGroup=point_group_name,
                                           LatticeCentering=centering_name)

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

        overall_statistics = dict(zip(keys, values))

        completentess = float(lines[3].split()[-1].replace('%', ''))
        overall_statistics['Completeness'] = completentess

        return overall_statistics

    def _compare_statistics(self, statistics, reference_statistics, space_group):
        self.assertEquals(round(statistics['Multiplicity'], 1), round(reference_statistics['<N>'], 1))
        self.assertEquals(round(statistics['Rpim'], 2), round(100.0 * reference_statistics['Rm'], 2))
        self.assertEquals(statistics['No. of Unique Reflections'], int(reference_statistics['Nunique']))
        self.assertDelta(round(statistics['Data Completeness'], 1), round(reference_statistics['Completeness'], 1),
                         0.5)
