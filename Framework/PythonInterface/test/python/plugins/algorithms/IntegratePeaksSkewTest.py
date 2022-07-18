# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import shutil
from os import path
from mantid.simpleapi import (IntegratePeaksSkew, CreatePeaksWorkspace, AddPeak, AnalysisDataService, CloneWorkspace)
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt

class IntegratePeaksSkewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument so can create a peak table
        cls.ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(2, 5, 11)  # nbanks, npix, nbins
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        # fake peak in spectra in middle bank 1 (centered on detID=37/spec=12 and TOF=3.5)
        cls.peak_1D = 2 * array([0, 0, 0, 1, 4, 6, 4, 1, 0, 0, 0])
        cls.ws.setY(12, cls.ws.readY(12) + cls.peak_1D)
        for ispec in [7, 11, 12, 13, 17, 22]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + cls.peak_1D)
            cls.ws.setE(ispec, sqrt(cls.ws.readY(ispec)))
        # make peak table
        cls.peaks = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace="peaks")
        AddPeak(PeaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=5, DetectorID=32)  # inside fake peak (bank 1)
        AddPeak(PeaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=5, DetectorID=27)  # outside fake peak (bank 1)
        AddPeak(peaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=5, DetectorID=62)  # middle bank 2 (no peak)
        # output file dir
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

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
        self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
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
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
            # check peak pos not moved
            self.assertEqual(out.column('DetID')[ipk], self.peaks.column('DetID')[ipk])
            self.assertAlmostEqual(pk.getTOF(), self.peaks.getPeak(ipk).getTOF(), delta=1e-10)
        # check other peaks not integrated
        self.assertEqual(out.getPeak(out.getNumberPeaks()-1).getIntensity(), 0)

    def test_integrate_use_nearest_peak_true_update_peak_position_true(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=True, UpdatePeakPosition=True,
                                 OutputWorkspace='out3')
        # check intensity/sigma of first two peaks equal (same peak integrated)
        # note intensity will be same now as peak position updated
        for ipk in range(2):
            pk = out.getPeak(ipk)
            self.assertAlmostEqual(pk.getIntensity(), 259054692.5, delta=1)
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
            # check peak pos moved to maximum
            self.assertEqual(out.column('DetID')[ipk], 37)
            self.assertAlmostEqual(pk.getTOF(), 5.5, delta=1e-10)
        # check other peaks not integrated
        self.assertEqual(out.getPeak(out.getNumberPeaks()-1).getIntensity(), 0)

    def test_print_output_file(self):
        out_file = path.join(self._test_dir, 'out.pdf')
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=False, OutputWorkspace='out4', OutputFile=out_file)
        # check output file saved
        self.assertTrue(path.exists(out_file))

    def test_peak_mask_validation_with_ncol_max(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out5', NColMax=2)
        for ipk, pk in enumerate(out):
            self.assertEqual(pk.getIntensity(), 0)

    def test_peak_mask_validation_with_ncol_max(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out6', NColMax=2)
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)

    def test_peak_mask_validation_with_nrow_max(self):
        out = IntegratePeaksSkew(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out6', NRowMax=3)
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)

    def test_peak_mask_validation_with_nvacancies(self):
        # make dataset with a vacancy of 1 pixel
        ws_clone = CloneWorkspace(InputWorkspace=self.ws)
        for ispec in [6, 8, 16, 18]:
            ws_clone.setY(ispec, ws_clone.readY(ispec) + 0.5 * self.peak_1D)
            ws_clone.setE(ispec, sqrt(ws_clone.readY(ispec)))
        for ispec in [12, 22]:
            ws_clone.setY(ispec, ws_clone.readY(0))
            ws_clone.setE(ispec, ws_clone.readE(0))

        # check vacancy with 1 pixel detected (and first peak therefore not integrated)
        out = IntegratePeaksSkew(InputWorkspace=ws_clone, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out6', NVacanciesMax=0, NPixPerVacancyMin=1)
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)

        # set npix per vacancies > 1 (should now integrate first peak)
        out = IntegratePeaksSkew(InputWorkspace=ws_clone, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out6', NVacanciesMax=0, NPixPerVacancyMin=2)
        self.assertNotEqual(out.getPeak(0).getIntensityOverSigma(), 0)

        # set nvacancies > 1 (should now integrate first peak)
        out = IntegratePeaksSkew(InputWorkspace=ws_clone, PeaksWorkspace=self.peaks, FractionalTOFWindow=0.3,
                                 IntegrateIfOnEdge=True, UseNearestPeak=False, UpdatePeakPosition=False,
                                 OutputWorkspace='out6', NVacanciesMax=1, NPixPerVacancyMin=1)
        self.assertNotEqual(out.getPeak(0).getIntensityOverSigma(), 0)


if __name__ == '__main__':
    unittest.main()
