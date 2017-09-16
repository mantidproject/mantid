# pylint: disable=no-init,invalid-name,too-many-locals,too-few-public-methods
from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *

# These tests check the correctness of the structure factor calculation for some common crystal structures.
# All structure factors for comparison have been calculated using VESTA 3.2.1, which is described in the following
# publication:
#
# K. Momma and F. Izumi, "VESTA 3 for three-dimensional visualization of crystal,
#           volumetric and morphology data," J. Appl. Crystallogr., 44, 1272-1276 (2011)
#
#           http://dx.doi.org/10.1107/S0021889811038970
#
# All crystal structure data have been acquired from http://www.crystallography.net/. DOIs of the original
# papers with the published structures are given in the tests.
#
# Isotropic thermal parameters are rounded or arbitrary (the tests are meant for checking the calculations only).


class ReflectionCheckingTest(stresstesting.MantidStressTest):
    def runTest(self):
        pass

    def getPeakIndex(self, peakTable, hkl):
        hklString = ' '.join([str(x) for x in hkl])

        for i in range(peakTable.rowCount()):
            r = peakTable.row(i)

            if r['HKL'] == hklString:
                return i

        return -1

    def checkReflections(self, peakTable, data, structureFactorPrecision = 1e-5):
        for reference in data:
            idx = self.getPeakIndex(peakTable, reference[0])

            # Make sure the reflection exists.
            self.assertTrue(idx != -1)

            currentPeak = peakTable.row(idx)

            self.assertEqual([int(x) for x in currentPeak['HKL'].split()], reference[0])
            self.assertDelta(float(currentPeak['d']), reference[1], 1e-4)

            fSquaredReference = reference[2] ** 2 * reference[3]
            print(reference[0], fSquaredReference, float(currentPeak['Intensity']))
            self.assertDelta(float(currentPeak['Intensity']) / fSquaredReference, 1.0, structureFactorPrecision)


class POLDICreatePeaksFromCellTestSiO2(ReflectionCheckingTest):
    """Structure factor check for:
            SiO2, 10.1107/S0108768105005240"""

    data = [
        ([1, 0, 0], 4.25588, 8.27544, 6),
        ([1, 0, -1], 3.34393, 22.1494, 6),
        ([0, 0, 3], 1.80193, 8.70574, 2),
        ([2, 2, 0], 1.22857, 14.4884, 3),
        ([4, -1, 4], 0.88902, 9.14321, 6)
    ]

    def runTest(self):
        peaks_SiO2 = PoldiCreatePeaksFromCell(
            SpaceGroup="P 32 2 1",
            Atoms="Si 0.4723 0.0 2/3 1.0 0.0075; O 0.416 0.2658 0.7881 1.0 0.0175",
            a=4.91427, c=5.4058, LatticeSpacingMin=0.885)

        self.assertEqual(peaks_SiO2.rowCount(), 118)

        self.checkReflections(peaks_SiO2, self.data)


class POLDICreatePeaksFromCellTestAl2O3(ReflectionCheckingTest):
    """Structure factor check for:
            Al2O3, 10.1107/S0021889890002382"""

    data = [
        ([1, 0, -2], 3.481144, 21.873, 6),
        ([1, 0, 4], 2.551773, 23.6714, 6),
        ([0, 0, 6], 2.165933, 68.8749, 2),
        ([5, -2, -5], 0.88880, 23.6113, 12)
    ]

    def runTest(self):
        peaks_Al2O3 = PoldiCreatePeaksFromCell(
            SpaceGroup="R -3 c",
            Atoms="Al 0 0 0.35216 1.0 0.009; O 0.30668 0 1/4 1.0 0.0125",
            a=4.7605, c=12.9956, LatticeSpacingMin=0.885)

        self.assertEqual(peaks_Al2O3.rowCount(), 44)

        self.checkReflections(peaks_Al2O3, self.data)


class POLDICreatePeaksFromCellTestFeTiO3(ReflectionCheckingTest):
    """Structure factor check for:
            FeTiO3, 10.1007/s00269-007-0149-7

        Note: Ti replaced by Zr"""

    data = [
        ([0, 0, 3], 4.6970, 2.0748, 2),
        ([1, 0, 1], 4.20559, 1.60512, 6),
        ([1, 0, 4], 2.75153, 76.1855, 6),
        ([5, -4, 6], 0.88986, 100.244, 6)
    ]

    def runTest(self):
        peaks_FeTiO3 = PoldiCreatePeaksFromCell(
            SpaceGroup="R -3",
            Atoms="Fe 0 0 0.35543 1.0 0.005; Zr 0 0 0.14643 1.0 0.004; O 0.31717 0.02351 0.24498 1.0 0.006",
            a=5.0881, c=14.091, LatticeSpacingMin=0.885)

        self.assertEqual(peaks_FeTiO3.rowCount(), 108)

        self.checkReflections(peaks_FeTiO3, self.data, 6e-5)


class POLDICreatePeaksFromCellTestCO(ReflectionCheckingTest):
    """Structure factor check for:
            CO, 10.1007/BF01339658

        Notes: Non-centrosymmetric, cubic, negative coordinates"""

    data = [
        ([1, 1, 0], 3.98101, 1.93291, 12),
        ([1, 1, -1], 3.25048, 40.6203, 4),
        ([2, 0, 0], 2.815, 37.248, 6),
        ([6, 2, 0], 0.89018, 9.45489, 12)
    ]

    def runTest(self):
        peaks_CO = PoldiCreatePeaksFromCell(
            SpaceGroup="P 21 3",
            Atoms="C -0.042 -0.042 -0.042 1.0 0.0125; O 0.067 0.067 0.067 1.0 0.0125",
            a=5.63, LatticeSpacingMin=0.885)

        self.assertEqual(peaks_CO.rowCount(), 91)

        self.checkReflections(peaks_CO, self.data, 1e-5)


class POLDICreatePeaksFromCellTestBetaQuartz(ReflectionCheckingTest):
    """Structure factor check for:
            SiO2 (beta-quartz, high temperature), 10.1127/ejm/2/1/0063

        Notes: Non-centrosymmetric, hexagonal, with coordinate 1/6"""

    data = [
        ([1, 0, 0], 4.32710, 7.74737, 6),
        ([1, 0, 1], 3.38996, 19.7652, 12),
        ([1, 0, 2], 2.30725, 2.96401, 12),
        ([1, 0, 6], 0.88968, 3.15179, 12)
    ]

    def runTest(self):
        peaks_betaSiO2 = PoldiCreatePeaksFromCell(
            SpaceGroup="P 62 2 2",
            Atoms="Si 1/2 0 0 1.0 0.025; O 0.41570 0.20785 1/6 1.0 0.058",
            a=4.9965, c=5.4546, LatticeSpacingMin=0.885)

        self.assertEqual(peaks_betaSiO2.rowCount(), 65)

        self.checkReflections(peaks_betaSiO2, self.data, 1e-5)
