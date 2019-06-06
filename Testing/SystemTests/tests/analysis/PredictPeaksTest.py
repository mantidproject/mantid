# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods
import systemtesting
from mantid.simpleapi import *
from mantid.geometry import CrystalStructure


# The reference data for these tests were created with PredictPeaks in the state at Release 3.5,
# if PredictPeaks changes significantly, both reference data and test may need to be adjusted.

# The WISH test has a data mismatch which might be caused by the 'old' code having a bug (issue #14105).
# The difference is that peaks may have different d-values because they are assigned to a different detector.
# Instead of using the CompareWorkspaces, only H, K and L are compared.
class PredictPeaksTestWISH(systemtesting.MantidSystemTest):
    def runTest(self):
        simulationWorkspace = CreateSimulationWorkspace(Instrument='WISH',
                                                        BinParams='0,1,2',
                                                        UnitX='TOF')

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(simulationWorkspace,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub.nxs')

        hkls_predicted = self._get_hkls(peaks)
        hkls_reference = self._get_hkls(reference)

        lists_match, message = self._compare_hkl_lists(hkls_predicted, hkls_reference)

        self.assertEqual(lists_match, True, message)

    def _get_hkls(self, peaksWorkspace):
        h_list = peaksWorkspace.column('h')
        k_list = peaksWorkspace.column('k')
        l_list = peaksWorkspace.column('l')

        return [(x, y, z) for x, y, z in zip(h_list, k_list, l_list)]

    def _compare_hkl_lists(self, lhs, rhs):
        if len(lhs) != len(rhs):
            return False, 'Lengths do not match: {} vs. {}'.format(len(lhs), len(rhs))

        lhs_sorted = sorted(lhs)
        rhs_sorted = sorted(rhs)

        for i in range(len(lhs)):
            if lhs_sorted[i] != rhs_sorted[i]:
                return False, 'Mismatch at position {}: {} vs. {}'.format(i, lhs_sorted[i], rhs_sorted[i])

        return True, None


class PredictPeaksTestTOPAZ(systemtesting.MantidSystemTest):
    def runTest(self):
        direc = config['instrumentDefinition.directory']
        xmlFile =  os.path.join(direc,'TOPAZ_Definition_2015-01-01.xml')
        simulationWorkspace = CreateSimulationWorkspace(Instrument=xmlFile,
                                                        BinParams='0,1,2',
                                                        UnitX='TOF')

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(simulationWorkspace,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub_topaz.nxs')

        simulationWorkspaceMatch = CompareWorkspaces(peaks, reference)

        self.assertTrue(simulationWorkspaceMatch[0])


class PredictPeaksCalculateStructureFactorsTest(systemtesting.MantidSystemTest):
    expected_num_peaks = 546

    def runTest(self):
        simulationWorkspace = CreateSimulationWorkspace(Instrument='WISH',
                                                        BinParams='0,1,2',
                                                        UnitX='TOF')

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')

        # Setting some random crystal structure. Correctness of structure factor calculations is ensured in the
        # test suite of StructureFactorCalculator and does not need to be tested here.
        simulationWorkspace.sample().setCrystalStructure(
            CrystalStructure('5.5 6.5 8.1', 'P m m m', 'Fe 0.121 0.234 0.899 1.0 0.01'))

        peaks = PredictPeaks(simulationWorkspace,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10,
                             CalculateStructureFactors=True)

        self.assertEqual(peaks.getNumberPeaks(), self.expected_num_peaks)

        for i in range(self.expected_num_peaks):
            peak = peaks.getPeak(i)
            self.assertLessThan(0.0, peak.getIntensity())

        peaks_no_sf = PredictPeaks(simulationWorkspace,
                                   WavelengthMin=0.5, WavelengthMax=6,
                                   MinDSpacing=0.5, MaxDSpacing=10,
                                   CalculateStructureFactors=False)

        for i in range(self.expected_num_peaks):
            peak = peaks_no_sf.getPeak(i)
            self.assertEqual(0.0, peak.getIntensity())
