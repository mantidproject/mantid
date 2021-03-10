# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy.testing import assert_allclose

from systemtesting import MantidSystemTest
from mantid.api import mtd
from mantid.simpleapi import (LoadNexus, IntegrateEllipsoids)


class IntegrateEllipsoidsTest(MantidSystemTest):
    r"""
    def requiredFiles(self):
        return ['TOPAZ_39037_bank29.nxs',  # input events
                'TOPAZ_39037_peaks_short.nxs']  # input peaks
    """
    def runTest(self):
        r"""Calculate intensities for a set of peaks. The first and secon peaks in the
        table corresponds to satellite peak HKL=(1.5, 1.5,0) and main peak (1,1,0)"""
        LoadNexus(Filename='TOPAZ_39037_bank29.nxs', OutputWorkspace='events')
        LoadNexus(Filename='TOPAZ_39037_peaks_short.nxs', OutputWorkspace='peaks_input')
        IntegrateEllipsoids(InputWorkspace='events',
                            PeaksWorkspace='peaks_input',
                            OutputWorkspace='peaks_output',
                            RegionRadius=0.14,
                            SpecifySize=True,
                            PeakSize=0.07,
                            BackgroundInnerSize=0.09,
                            BackgroundOuterSize=0.11,
                            CutoffIsigI=5.0,
                            AdaptiveQBackground=True,
                            AdaptiveQMultiplier=0.001,
                            UseOnePercentBackgroundCorrection=False)

        table = mtd['peaks_output']
        # intensities for the first two peaks
        assert_allclose(table.column('Intens')[0:2], [938, 13936], atol=1.0)
