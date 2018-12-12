# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from systemtesting import MantidSystemTest

from mantid.simpleapi import (ConvertUnits, LoadRaw, FilterPeaks, PredictPeaks, SetUB, SaveIsawPeaks)
from mantid import config
import numpy as np
import os


class WISHSingleCrystalPeakPredictionTest(MantidSystemTest):
    """
    At the time of writing WISH users rely quite heavily on the PredictPeaks
    algorithm. As WISH has tubes rather than rectangular detectors sometimes
    peaks fall between the gaps in the tubes.

    Here we check that PredictPeaks works on a real WISH dataset & UB. This also
    includes an example of a peak whose center is predicted to fall between two
    tubes.
    """

    def requiredFiles(self):
        return ["WISH00038237.raw", "WISHPredictedSingleCrystalPeaks.nxs"]

    def requiredMemoryMB(self):
        # Need lots of memory for full WISH dataset
        return 16000

    def cleanup(self):
        try:
            os.path.remove(self._peaks_file)
        except:
            pass

    def runTest(self):
        ws = LoadRaw(Filename='WISH00038237.raw', OutputWorkspace='38237')
        ws = ConvertUnits(ws, 'dSpacing', OutputWorkspace='38237')
        UB = np.array([[-0.00601763,  0.07397297,  0.05865706],
                       [ 0.05373321,  0.050198,   -0.05651455],
                       [-0.07822144,  0.0295911,  -0.04489172]])

        SetUB(ws, UB=UB)

        self._peaks = PredictPeaks(ws, WavelengthMin=0.1, WavelengthMax=100,
                                   OutputWorkspace='peaks')
        # We specifically want to check peak -5 -1 -7 exists, so filter for it
        self._filtered = FilterPeaks(self._peaks, "h^2+k^2+l^2", 75, '=',
                                     OutputWorkspace='filtered')

        SaveIsawPeaks(self._peaks, Filename='WISHSXReductionPeaksTest.peaks')

    def validate(self):
        self.assertEqual(self._peaks.rowCount(), 510)
        self.assertEqual(self._filtered.rowCount(), 6)

        # The peak at [-5 -1 -7] is known to fall between the gaps of WISH's tubes
        # Specifically check this one is predicted to exist because past bugs have
        # been found in the ray tracing.
        Peak = namedtuple('Peak', ('DetID', 'BankName', 'h', 'k', 'l'))
        expected = Peak(DetID=9202086, BankName='WISHpanel09', h=-5.0, k=-1.0, l=-7.0)
        expected_peak_found = False
        for row in self._filtered:
            peak = Peak(DetID=row['DetID'], BankName=row['BankName'], h=row['h'], k=row['k'], l=row['l'])
            if peak == expected:
                expected_peak_found = True
                break
        #endfor
        self.assertTrue(expected_peak_found, msg="Peak at {} expected but it was not found".format(expected))
        self._peaks_file = os.path.join(config['defaultsave.directory'], 'WISHSXReductionPeaksTest.peaks')
        self.assertTrue(os.path.isfile(self._peaks_file))

        return self._peaks.name(), "WISHPredictedSingleCrystalPeaks.nxs"
