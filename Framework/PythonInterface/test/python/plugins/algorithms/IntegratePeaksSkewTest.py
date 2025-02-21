# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import shutil
from os import path
from mantid.simpleapi import (
    IntegratePeaksSkew,
    CreatePeaksWorkspace,
    AddPeak,
    AnalysisDataService,
    CloneWorkspace,
    LoadEmptyInstrument,
    SetInstrumentParameter,
    ConvertUnits,
)
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt
import json


class IntegratePeaksSkewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument with RectangularDetector banks and create a peak table
        cls.ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(2, 5, 11)  # nbanks, npix, nbins
        AnalysisDataService.addOrReplace("ws_rect", cls.ws)
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        # fake peak in spectra in middle bank 1 (centered on detID=37/spec=12 and TOF=5)
        cls.peak_1D = 2 * array([0, 0, 0, 1, 4, 6, 4, 1, 0, 0, 0])
        cls.ws.setY(12, cls.ws.readY(12) + cls.peak_1D)
        for ispec in [7, 11, 12, 13, 17, 22]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + cls.peak_1D)
            cls.ws.setE(ispec, sqrt(cls.ws.readY(ispec)))
        # Add back-to-back exponential params
        for param in ["A", "B", "S"]:
            SetInstrumentParameter(cls.ws, ParameterName=param, Value="1")
        # make peak table
        # 0) inside fake peak (bank 1)
        # 1) outside fake peak (bank 1)
        # 2) middle bank 2 (no peak)
        cls.peaks = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace="peaks")
        for ipk, detid in enumerate([32, 27, 62]):
            AddPeak(PeaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=4, DetectorID=detid)
            cls.peaks.getPeak(ipk).setHKL(ipk, ipk, ipk)

        # Load empty WISH with ComponentArray banks
        cls.ws_comp_arr = LoadEmptyInstrument(InstrumentName="WISH", OutputWorkspace="WISH")
        cls.ws_comp_arr.getAxis(0).setUnit("TOF")
        cls.peaks_comp_arr = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws_comp_arr, NumberOfPeaks=0, OutputWorkspace="peaks_comp_arr")
        for ipk, detid in enumerate([10707000, 10707511, 10100255, 9707255, 5104246]):
            AddPeak(PeaksWorkspace=cls.peaks_comp_arr, RunWorkspace=cls.ws_comp_arr, TOF=1e4, DetectorID=detid)
            cls.peaks_comp_arr.getPeak(ipk).setHKL(ipk, ipk, ipk)

        # output file dir
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

    def test_integrate_on_edge_option(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=False,
            OutputWorkspace="out0",
        )
        # check peaks in bank 1 were not integrated (mask touches masked pixel)
        for ipk, pk in enumerate(out):
            self.assertEqual(pk.getIntensity(), 0)
            self.assertEqual(pk.getPeakShape().shapeName(), "none")

    def test_integrate_on_edge_option_respects_detector_masking(self):
        ws_masked = CloneWorkspace(InputWorkspace=self.ws)
        det_info = ws_masked.detectorInfo()
        det_info.setMasked(det_info.indexOf(self.peaks.getPeak(0).getDetectorID()), True)
        out = IntegratePeaksSkew(
            InputWorkspace=ws_masked,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=False,
            NRowsEdge=0,
            NColsEdge=0,
            OutputWorkspace="out0_masked",
        )
        # check peaks in bank 1 were not integrated (mask touches edge)
        self.assertEqual(out.getPeak(0).getIntensity(), 0)

    def test_integrate_use_nearest_peak_false_update_peak_position_false(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out1",
        )
        # check intensity of first peak (only valid peak)
        ipk = 0
        pk = out.getPeak(ipk)
        self.assertAlmostEqual(pk.getIntensity(), 578738736.0, delta=1)
        self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
        # check peak pos not moved
        self.assertEqual(out.column("DetID")[ipk], self.peaks.column("DetID")[ipk])
        self.assertAlmostEqual(pk.getTOF(), self.peaks.getPeak(ipk).getTOF(), delta=1e-10)
        # check other peaks not integrated
        for ipk in range(1, out.getNumberPeaks()):
            self.assertEqual(out.getPeak(ipk).getIntensity(), 0)

    def test_integrate_use_nearest_peak_false_update_peak_position_false_with_resolution_params(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            BackscatteringTOFResolution=0.3,
            ThetaWidth=0.006,
            ScaleThetaWidthByWavelength=False,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out11",
        )
        out_scaled = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            BackscatteringTOFResolution=0.3,
            ThetaWidth=0.6,
            ScaleThetaWidthByWavelength=True,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out12",
        )
        # check intensity of first peak - should be same as TOF window comes out as ~0.3 (as in other tests)
        for pk_ws in [out, out_scaled]:
            self.assertAlmostEqual(pk_ws.getPeak(0).getIntensityOverSigma(), 12.7636, delta=1e-3)

    def test_integrate_use_nearest_peak_false_update_peak_position_false_with_back_to_back_params(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            GetTOFWindowFromBackToBackParams=True,
            NFWHM=4,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out11",
        )
        # check intensity of first peak - should be same as TOF window comes out as ~0.3 (as in other tests)
        self.assertAlmostEqual(out.getPeak(0).getIntensityOverSigma(), 12.7636, delta=1e-3)

    def test_integrate_use_nearest_peak_true_update_peak_position_false(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=True,
            UpdatePeakPosition=False,
            OutputWorkspace="out2",
        )
        # check intensity/sigma of first two peaks equal (same peak integrated)
        # note intensity will be different due to lorz factor as position not updated
        for ipk in range(2):
            pk = out.getPeak(ipk)
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
            # check peak pos not moved
            self.assertEqual(out.column("DetID")[ipk], self.peaks.column("DetID")[ipk])
            self.assertAlmostEqual(pk.getTOF(), self.peaks.getPeak(ipk).getTOF(), delta=1e-10)
        # check other peaks not integrated
        self.assertEqual(out.getPeak(out.getNumberPeaks() - 1).getIntensity(), 0)

    def test_integrate_use_nearest_peak_true_update_peak_position_true(self):
        ws_dspac = "ws_dSpac"
        ConvertUnits(InputWorkspace=self.ws, OutputWorkspace=ws_dspac, Target="dSpacing")
        for ws in [self.ws, ws_dspac]:
            out = IntegratePeaksSkew(
                InputWorkspace=self.ws,
                PeaksWorkspace=self.peaks,
                ThetaWidth=0,
                BackscatteringTOFResolution=0.3,
                IntegrateIfOnEdge=True,
                UseNearestPeak=True,
                UpdatePeakPosition=True,
                OutputWorkspace="out3",
            )
            # check intensity/sigma of first two peaks equal (same peak integrated)
            # note intensity will be same now as peak position updated
            for ipk in range(2):
                pk = out.getPeak(ipk)
                self.assertAlmostEqual(pk.getIntensity(), 259054692.5, delta=1)
                self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7636, delta=1e-3)
                # check peak pos moved to maximum
                self.assertEqual(out.column("DetID")[ipk], 37)
                self.assertAlmostEqual(pk.getTOF(), 5.5, delta=1e-10)
                # check that HKL have been stored
                hkl = pk.getHKL()
                for miller_index in hkl:
                    self.assertEqual(miller_index, ipk)
            # check other peaks not integrated
            self.assertEqual(out.getPeak(out.getNumberPeaks() - 1).getIntensity(), 0)

    def test_print_output_file(self):
        out_file = path.join(self._test_dir, "out.pdf")
        IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=False,
            OutputWorkspace="out4",
            OutputFile=out_file,
        )
        # check output file saved
        self.assertTrue(path.exists(out_file))

    def test_peak_mask_validation_with_ncol_max(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out5",
            NColMax=2,
        )
        for ipk, pk in enumerate(out):
            self.assertEqual(pk.getIntensity(), 0)

    def test_peak_min_number_tof_bins(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out5",
            NTOFBinsMin=10,
        )
        for ipk, pk in enumerate(out):
            self.assertEqual(pk.getIntensity(), 0)

    def test_peak_mask_validation_after_ncol_max_increase(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out6",
            NColMax=3,
        )
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)
        # increase ncol to accept peak mask
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out6",
            NColMax=4,
        )
        self.assertGreater(out.getPeak(0).getIntensityOverSigma(), 0)

    def test_peak_mask_validation_with_nrow_max(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out7",
            NRowMax=2,
        )
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)
        # increase nrow to accept peak mask
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out7",
            NRowMax=3,
        )
        self.assertGreater(out.getPeak(0).getIntensityOverSigma(), 0)

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
        out = IntegratePeaksSkew(
            InputWorkspace=ws_clone,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            NTOFBinsMin=2,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out8",
            NVacanciesMax=0,
            NPixPerVacancyMin=1,
        )
        self.assertEqual(out.getPeak(0).getIntensityOverSigma(), 0)

        # set npix per vacancies > 1 (should now integrate first peak)
        out = IntegratePeaksSkew(
            InputWorkspace=ws_clone,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            NTOFBinsMin=2,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out8",
            NVacanciesMax=0,
            NPixPerVacancyMin=2,
        )
        self.assertGreater(out.getPeak(0).getIntensityOverSigma(), 0)

        # set nvacancies > 1 (should now integrate first peak)
        out = IntegratePeaksSkew(
            InputWorkspace=ws_clone,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            NTOFBinsMin=2,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            OutputWorkspace="out8",
            NVacanciesMax=1,
            NPixPerVacancyMin=1,
        )
        self.assertGreater(out.getPeak(0).getIntensityOverSigma(), 0)

    def test_integration_with_non_square_window_nrows_ncols(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            NTOFBinsMin=1,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            NRows=5,
            NCols=3,
            NRowMax=3,
            NColMax=3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=True,
            UpdatePeakPosition=False,
            OutputWorkspace="out8",
        )
        self.assertGreater(out.getPeak(0).getIntensityOverSigma(), 0)
        self.assertAlmostEqual(out.getPeak(0).getIntensityOverSigma(), 8.8002, delta=1e-4)
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            NTOFBinsMin=1,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            NRows=3,
            NCols=5,
            NRowMax=3,
            NColMax=3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=True,
            NPixMin=1,
            UpdatePeakPosition=False,
            OutputWorkspace="out9",
        )
        self.assertAlmostEqual(out.getPeak(0).getIntensityOverSigma(), 5.46125, delta=1e-4)  # only one pixel in peak

    def test_lorentz_correction_false(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            LorentzCorrection=False,
            OutputWorkspace="out10",
        )
        # check intensity of first peak (only valid peak)
        pk = out.getPeak(0)
        self.assertAlmostEqual(pk.getIntensity(), 224, delta=1e-2)
        self.assertAlmostEqual(pk.getIntensityOverSigma(), 12.7635, delta=1e-4)

    def test_shapeof_valid_peaks(self):
        out = IntegratePeaksSkew(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            ThetaWidth=0,
            BackscatteringTOFResolution=0.3,
            IntegrateIfOnEdge=True,
            UseNearestPeak=False,
            UpdatePeakPosition=False,
            LorentzCorrection=False,
            OutputWorkspace="out10",
        )
        # check shape of the only valid peak
        pk = out.getPeak(0)
        self.assertEqual(pk.getPeakShape().shapeName(), "detectorbin")
        self.assertEqual(pk.getPeakShape().algorithmName(), "IntegratePeaksSkew")
        pk_shape_dict = json.loads(pk.getPeakShape().toJSON())
        self.assertEqual(len(pk_shape_dict["detectors"]), 6)
        for det in pk_shape_dict["detectors"]:
            self.assertEqual(det["startX"], 3)
            self.assertEqual(det["endX"], 8)
        for i in [1, 2]:
            self.assertEqual(out.getPeak(i).getPeakShape().shapeName(), "none")


if __name__ == "__main__":
    unittest.main()
