# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import (
    LoadIsawPeaks,
    AnalysisDataService,
)


class LoadIsawPeaksWithModVecsTest(systemtesting.MantidSystemTest):
    def tearDown(self):
        AnalysisDataService.clear()

    def runTest(self):
        mod_vec = [0, 0, 0.2]

        ref = LoadIsawPeaks(Filename="TOPAZ_modulation.peaks")

        ref_vec = ref.sample().getOrientedLattice().getModVec(0)

        for i in range(3):
            self.assertAlmostEqual(ref_vec[i], mod_vec[i], 2)
