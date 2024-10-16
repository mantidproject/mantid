# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import CountReflections
from SortHKLTest import HKLStatisticsTestMixin


class CountReflectionsTest(HKLStatisticsTestMixin, systemtesting.MantidSystemTest):
    """
    This systemtest follows the same principle as the one for SortHKL. It loads data,
    computes statistics and checks them against reference data obtained from another
    software package (SORTAV, see SortHKLTest.py for a reference).
    """

    def runTest(self):
        self._init_test_data()
        self.test_CountReflections()

    def test_CountReflections(self):
        for space_group in self._space_groups:
            ub_parameters = self._load_ub_parameters(space_group)
            reflections = self._load_reflections(space_group, ub_parameters)
            reference_statistics = self._load_reference_statistics(space_group)

            statistics = self._run_count_reflections(reflections, space_group)

            self._compare_statistics(statistics._asdict(), reference_statistics)

    def _run_count_reflections(self, reflections, space_group):
        point_group = self._get_point_group(space_group).getHMSymbol()
        centering = space_group[0]

        return CountReflections(
            InputWorkspace=reflections, PointGroup=point_group, LatticeCentering=centering, MinDSpacing=0.5, MaxDSpacing=10.0
        )

    def _compare_statistics(self, statistics, reference_statistics):
        self.assertEqual(round(statistics["Redundancy"], 1), round(reference_statistics["<N>"], 1))
        self.assertEqual(statistics["UniqueReflections"], int(reference_statistics["Nunique"]))
        self.assertDelta(round(statistics["Completeness"] * 100.0, 1), round(reference_statistics["Completeness"], 1), 0.5)
