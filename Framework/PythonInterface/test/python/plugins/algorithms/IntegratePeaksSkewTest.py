# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (IntegratePeaksSkew, CreatePeaksWorkspace, AddPeak, AnalysisDataService)
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt

class IntegratePeaksSkewTest(unittest.TestCase):

    def setUp(self):
        # load empty instrument so can create a peak table
        self.ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(2, 5, 11)  # nbanks, npix, nbins
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")
        # fake peak in spectra in middle bank 1 (centered on detID=37/spec=12 and TOF=3.5)
        peak_1D = 2 * array([0, 0, 0, 1, 4, 6, 4, 1, 0, 0, 0])
        self.ws.setY(12, self.ws.readY(12) + peak_1D)
        ispecs = [7, 11, 12, 13, 17, 22]
        for ispec in ispecs:
            self.ws.setY(ispec, self.ws.readY(ispec) + peak_1D)
            self.ws.setE(ispec, sqrt(self.ws.readY(ispec)))
        # make peak table
        self.peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks")
        AddPeak(PeaksWorkspace=self.peaks, RunWorkspace=self.ws, TOF=5, DetectorID=32)  # inside fake peak (bank 1)
        AddPeak(PeaksWorkspace=self.peaks, RunWorkspace=self.ws, TOF=5, DetectorID=27)  # outside fake peak (bank 1)
        AddPeak(peaksWorkspace=self.peaks, RunWorkspace=self.ws, TOF=5, DetectorID=62)  # middle bank 2 (no peak)

    def tearDown(self):
        AnalysisDataService.clear()

    def test_integrate_on_edge_option(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                           IntegrateIfOnEdge=False, OutputWorkspace='out0')
        # check peaks in bank 1 were not integrated (mask touches edge)
        for ipk, pk in enumerate(out):
            self.assertEqual(pk.getIntensity(), 0)

    def test_integrate_use_nearest_peak_false_update_peak_position_false(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out1')
        # check intensity of first peak (only valid peak)
        ipk = 0
        pk = out.getPeak(ipk)
        self.assertAlmostEqual(pk.getIntensity(), 237051386.2, delta=1)
        self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.3495, delta=1e-3)
        # check peak pos not moved
        self.assertEqual(out.column('DetID')[ipk], self.peaks.column('DetID')[ipk])
        self.assertAlmostEqual(pk.getTOF(), self.peaks.getPeak(ipk).getTOF(), delta=1e-10)
        # check other peaks not integrated
        for ipk in range(1, out.getNumberPeaks()):
            self.assertEqual(out.getPeak(ipk).getIntensity(), 0)

    def test_integrate_use_nearest_peak_true_update_peak_position_false(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=True, UpdatePeakPosition=False,
                                 OutputWorkspace='out2')
        # check intensity/sigma of first two peaks equal (same peak integrated)
        # note intensity will be different due to lorz factor as position not updated
        for ipk in range(2):
            pk = out.getPeak(ipk)
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.3495, delta=1e-3)
            # check peak pos not moved
            self.assertEqual(out.column('DetID')[ipk], self.peaks.column('DetID')[ipk])
            self.assertAlmostEqual(pk.getTOF(), self.peaks.getPeak(ipk).getTOF(), delta=1e-10)
        # check other peaks not integrated
        self.assertEqual(out.getPeak(out.getNumberPeaks()-1).getIntensity(), 0)

    def test_integrate_use_nearest_peak_true_update_peak_position_true(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=True, UpdatePeakPosition=True,
                                 OutputWorkspace='out2')
        # check intensity/sigma of first two peaks equal (same peak integrated)
        # note intensity will be same now as peak position updated
        for ipk in range(2):
            pk = out.getPeak(ipk)
            self.assertAlmostEqual(pk.getIntensity(), 259054692.5, delta=1)
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.3495, delta=1e-3)
            # check peak pos moved to maximum
            self.assertEqual(out.column('DetID')[ipk], 37)
            self.assertAlmostEqual(pk.getTOF(), 5.5, delta=1e-10)
        # check other peaks not integrated
        self.assertEqual(out.getPeak(out.getNumberPeaks()-1).getIntensity(), 0)



if __name__ == '__main__':
    unittest.main()
