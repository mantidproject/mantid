# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid import config
from mantid.simpleapi import CORELLIPowderCalibrationGenerate

class GenerateTest(systemtesting.MantidSystemTest):
    r"""
    Test creating a calibration
    """

    def requiredFiles(self):
        r"""
        Runs for standard sample CsLaNb2O7
        """
        return ['CORELLI_124026-124035_bank42.nxs']

    def runTest(self):
        raw_events = LoadEventNexus(Filename='CORELLI_59313-59320_bank42.nxs')
        reference_spacings = [1.1085, 1.2458, 1.3576, 1.6374, 1.9200, 3.1353]

        CORELLIPowderCalibrationGenerate(InputWorkspace='raw_events',
                                         OuputWorkspace='events_cal',
                                         PeakFunction='Gaussian',
                                         PeakPositions=reference_spacings)