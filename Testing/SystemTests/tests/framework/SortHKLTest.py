# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
import json
from mantid.api import FileFinder
from mantid.simpleapi import CreatePeaksWorkspace, CreateSimulationWorkspace, LoadHKL, SetUB, SortHKL
from mantid.geometry import PointGroupFactory


class HKLStatisticsTestMixin(object):
    def _init_test_data(self):
        self._ws = CreateSimulationWorkspace(
            Instrument="TOPAZ", BinParams="0,10000,20000", UnitX="TOF", OutputWorkspace="topaz_instrument_workspace"
        )

        self._space_groups = ["Pm-3m", "P4_mmm", "Pmmm", "Im-3m", "Fm-3m", "I4_mmm", "Cmmm", "Immm", "Fmmm"]

        self._centering_map = {"P": "Primitive", "I": "Body centred", "F": "All-face centred", "C": "C-face centred"}

        self._base_directory = "SortHKL/"
        self._template_hkl = "reflections_{0}.hkl"
        self._template_ub = "ub_parameters_{0}.json"
        self._template_statistics = "statistics_{0}.txt"

    def _load_ub_parameters(self, space_group):
        filename = FileFinder.Instance().getFullPath(self._base_directory + self._template_ub.format(space_group))

        ub_parameters = {}
        with open(filename, "r") as ub_parameter_file:
            raw_ub_parameters = json.load(ub_parameter_file)

            # Mantid functions don't seem to like unicode so the dict is re-written
            ub_parameters.update(dict([(str(x), y if isinstance(y, float) else str(y)) for x, y in raw_ub_parameters.items()]))

        return ub_parameters

    def _load_reflections(self, space_group, ub_parameters):
        filename = self._base_directory + self._template_hkl.format(space_group)

        hkls_from_file = LoadHKL(Filename=filename)

        actual_hkls = CreatePeaksWorkspace(InstrumentWorkspace=self._ws, NumberOfPeaks=0)

        for i in range(hkls_from_file.getNumberPeaks()):
            actual_hkls.addPeak(hkls_from_file.getPeak(i))

        SetUB(Workspace=actual_hkls, **ub_parameters)

        return actual_hkls

    def _get_point_group(self, space_group):
        return PointGroupFactory.createPointGroup(space_group[1:].replace("_", "/"))

    def _load_reference_statistics(self, space_group):
        filename = FileFinder.Instance().getFullPath(self._base_directory + self._template_statistics.format(space_group))

        lines = []
        with open(filename, "r") as statistics_file:
            for line in statistics_file:
                lines.append(line)

        keys = lines[0].split()
        values = [float(x) for x in lines[2].split()[2:]]

        overall_statistics = dict(list(zip(keys, values)))

        completentess = float(lines[3].split()[-1].replace("%", ""))
        overall_statistics["Completeness"] = completentess

        return overall_statistics


class SortHKLTest(HKLStatisticsTestMixin, systemtesting.MantidSystemTest):
    """System test for SortHKL

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
    """

    def runTest(self):
        self._init_test_data()
        self.test_SortHKLStatistics()

    def test_SortHKLStatistics(self):
        for space_group in self._space_groups:
            ub_parameters = self._load_ub_parameters(space_group)
            reflections = self._load_reflections(space_group, ub_parameters)
            statistics, sorted_hkls = self._run_sort_hkl(reflections, space_group)
            reference_statistics = self._load_reference_statistics(space_group)

            self._compare_statistics(statistics, reference_statistics)
            # No need to check since intensities do no change
            # self._check_sorted_hkls_consistency(sorted_hkls, space_group)

    def _run_sort_hkl(self, reflections, space_group):
        point_group_name = self._get_point_group(space_group).getName()
        centering_name = self._centering_map[space_group[0]]

        # pylint: disable=unused-variable
        sorted_hkls, chi2, statistics, equivInten = SortHKL(
            InputWorkspace=reflections, PointGroup=point_group_name, LatticeCentering=centering_name
        )

        return statistics.row(0), sorted_hkls

    def _compare_statistics(self, statistics, reference_statistics):
        self.assertEqual(round(statistics["Multiplicity"], 1), round(reference_statistics["<N>"], 1))
        self.assertEqual(round(statistics["Rpim"], 2), round(100.0 * reference_statistics["Rm"], 2))
        self.assertEqual(statistics["No. of Unique Reflections"], int(reference_statistics["Nunique"]))
        self.assertDelta(round(statistics["Data Completeness"], 1), round(reference_statistics["Completeness"], 1), 0.5)

    def _check_sorted_hkls_consistency(self, sorted_hkls, space_group):
        peaks = [sorted_hkls.getPeak(i) for i in range(sorted_hkls.getNumberPeaks())]

        point_group = self._get_point_group(space_group)

        unique_map = dict()
        for peak in peaks:
            unique = point_group.getReflectionFamily(peak.getHKL())
            if unique not in unique_map:
                unique_map[unique] = [peak]
            else:
                unique_map[unique].append(peak)

        # pylint: disable=unused-variable
        for unique_hkl, equivalents in unique_map.items():
            if len(equivalents) > 1:
                reference_peak = equivalents[0]
                for peak in equivalents[1:]:
                    self.assertEqual(peak.getIntensity(), reference_peak.getIntensity())
                    self.assertEqual(peak.getSigmaIntensity(), reference_peak.getSigmaIntensity())
