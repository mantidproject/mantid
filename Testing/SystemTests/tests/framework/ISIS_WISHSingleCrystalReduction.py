# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from systemtesting import MantidSystemTest

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (ConvertUnits, LoadRaw, FilterPeaks, PredictPeaks, SetUB, SaveIsawPeaks, MaskBTP,
                              LoadEmptyInstrument, AddPeak, CreatePeaksWorkspace, CreateMDWorkspace, IntegratePeaksMD)
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
        return 27000

    def cleanup(self):
        ADS.clear()
        try:
            os.path.remove(self._peaks_file)
        except:
            pass

    def runTest(self):
        ws = LoadRaw(Filename='WISH00038237.raw', OutputWorkspace='38237')
        ws = ConvertUnits(ws, 'dSpacing', OutputWorkspace='38237')
        UB = np.array([[-0.00601763, 0.07397297, 0.05865706],
                       [0.05373321, 0.050198, -0.05651455],
                       [-0.07822144, 0.0295911, -0.04489172]])

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
        BasicPeak = namedtuple('Peak', ('DetID', 'BankName', 'h', 'k', 'l'))
        expected = BasicPeak(DetID=9202086, BankName='WISHpanel09', h=-5.0, k=-1.0, l=-7.0)
        expected_peak_found = False
        peak_count = self._filtered.rowCount()
        for i in range(peak_count):  # iterate of the table representation of the PeaksWorkspace
            peak_row = self._filtered.row(i)
            peak = BasicPeak(**{k: peak_row[k] for k in BasicPeak._fields})
            if peak == expected:
                expected_peak_found = True
                break
        # endfor
        self.assertTrue(expected_peak_found, msg="Peak at {} expected but it was not found".format(expected))
        self._peaks_file = os.path.join(config['defaultsave.directory'], 'WISHSXReductionPeaksTest.peaks')
        self.assertTrue(os.path.isfile(self._peaks_file))

        return self._peaks.name(), "WISHPredictedSingleCrystalPeaks.nxs"


class WISHPeakIntegrationRespectsMaskingTest(MantidSystemTest):
    """
    This tests that IntegratePeaksMD correctly ignores peaks at tube ends and that a custom masking for tubes
    adjacent to the beam in and out is respected by IntegratePeaksMD
    """

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        # Load Empty Instrument
        ws = LoadEmptyInstrument(InstrumentName='WISH', OutputWorkspace='WISH')
        axis = ws.getAxis(0)
        axis.setUnit("TOF")  # need this to add peak to table
        # CreatePeaksWorkspace with peaks in specific detectors
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace='peaks')
        AddPeak(PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000,
                DetectorID=1707204, Height=521, BinCount=0)  # pixel in first tube in panel 1
        AddPeak(PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000,
                DetectorID=1400510, Height=1, BinCount=0)  # pixel at top of a central tube in panel 1
        AddPeak(PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000,
                DetectorID=1408202, Height=598, BinCount=0)  # pixel in middle of bank 1 (not near edge)
        AddPeak(PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000,
                DetectorID=1100173, Height=640, BinCount=0)  # pixel in last tube of panel 1 (next to panel 2)
        # create dummy MD workspace for integration (don't need data as checking peak shape)
        MD = CreateMDWorkspace(Dimensions='3', Extents='-1,1,-1,1,-1,1',
                               Names='Q_lab_x,Q_lab_y,Q_lab_z', Units='U,U,U',
                               Frames='QLab,QLab,QLab',
                               SplitInto='2', SplitThreshold='50')
        # Integrate peaks masking all pixels at tube end (built into IntegratePeaksMD)
        self._peaks_pixels = IntegratePeaksMD(InputWorkspace=MD, PeakRadius='0.02', PeaksWorkspace=peaks,
                                              IntegrateIfOnEdge=False, OutputWorkspace='peaks_pixels',
                                              MaskEdgeTubes=False)
        # Apply masking to specific tubes next to beam in/out (subset of all edge tubes) and integrate again
        MaskBTP(Workspace='peaks', Bank='5-6', Tube='152')
        MaskBTP(Workspace='peaks', Bank='1,10', Tube='1')
        self._peaks_pixels_beamTubes = IntegratePeaksMD(InputWorkspace='MD', PeakRadius='0.02', PeaksWorkspace=peaks,
                                                        IntegrateIfOnEdge=False,
                                                        OutputWorkspace='peaks_pixels_beamTubes',
                                                        MaskEdgeTubes=False)
        # Integrate masking all edge tubes
        self._peaks_pixels_edgeTubes = IntegratePeaksMD(InputWorkspace='MD', PeakRadius='0.02', PeaksWorkspace='peaks',
                                                        IntegrateIfOnEdge=False,
                                                        OutputWorkspace='peaks_pixels_edgeTubes',
                                                        MaskEdgeTubes=True)

    def validate(self):
        # test which peaks were not integrated due to being on edge (i.e. shape = 'none')

        self.assertListEqual([pk.getPeakShape().shapeName() for pk in self._peaks_pixels],
                              ['spherical', 'none', 'spherical', 'spherical'])
        self.assertListEqual([pk.getPeakShape().shapeName() for pk in self._peaks_pixels_beamTubes],
                              ['none', 'none', 'spherical', 'spherical'])
        self.assertListEqual([pk.getPeakShape().shapeName() for pk in self._peaks_pixels_edgeTubes],
                              ['none', 'none', 'spherical', 'none'])
