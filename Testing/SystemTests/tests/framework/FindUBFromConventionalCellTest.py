# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting

import numpy as np

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import FindUBFromConventionalCell, Load

# Real macromolecular peaks data. The lattice is orthorhombic P with these
# conventional-cell parameters.
PEAKS_FILE = "MANDI_Macromolecular_Orthorhombic_P.nxs"
A, B, C = 85.2, 89.6, 110.9
ALPHA = BETA = GAMMA = 90.0


class FindUBFromConventionalCellTest(systemtesting.MantidSystemTest):
    """
    Recover the UB matrix of a macromolecular crystal from real peaks data and the known
    conventional cell.

    This exercises the algorithm on a large cell, where the reciprocal-lattice spacings are small and
    the orientation search is correspondingly demanding, rather than on the synthetic lattices used by
    the unit tests.
    """

    def cleanup(self):
        ADS.clear()

    def requiredFiles(self):
        return [PEAKS_FILE]

    def runTest(self):
        ws = Load(Filename=PEAKS_FILE, OutputWorkspace="peaks")

        # RandomSeed fixes the spherical-cap refinement so the result is reproducible.
        FindUBFromConventionalCell(
            PeaksWorkspace=ws,
            a=A,
            b=B,
            c=C,
            alpha=ALPHA,
            beta=BETA,
            gamma=GAMMA,
            Centering="P",
            NumAzimuth=240,
            NumPolar=120,
            CapAngleDeg=10.0,
            CapSamples=1000,
            NumPsi=720,
            RandomSeed=1234,
        )

        self.peaks = ws

    def validate(self):
        ol = self.peaks.sample().getOrientedLattice()

        self.assertAlmostEqual(ol.a(), A, delta=0.5)
        self.assertAlmostEqual(ol.b(), B, delta=0.5)
        self.assertAlmostEqual(ol.c(), C, delta=0.5)
        self.assertAlmostEqual(ol.alpha(), ALPHA, delta=1.0)
        self.assertAlmostEqual(ol.beta(), BETA, delta=1.0)
        self.assertAlmostEqual(ol.gamma(), GAMMA, delta=1.0)

        # The recovered UB must index most of the peaks. A minority of the peaks in a real data set are
        # spurious, so require only that fewer than 20% fail to index within 0.3 r.l.u. on every index.
        UB = ol.getUB().copy()
        q_vectors = np.array(
            [
                [
                    self.peaks.getPeak(i).getQSampleFrame().X(),
                    self.peaks.getPeak(i).getQSampleFrame().Y(),
                    self.peaks.getPeak(i).getQSampleFrame().Z(),
                ]
                for i in range(self.peaks.getNumberPeaks())
            ]
        )
        hkl = np.linalg.solve(UB, q_vectors.T).T / (2.0 * np.pi)
        err = np.abs(hkl - np.rint(hkl))
        fail_fraction = np.mean(np.any(err > 0.3, axis=1))

        self.assertLessThan(fail_fraction, 0.2)
