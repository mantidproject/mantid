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
from plugins.algorithms.IntegratePeaksSkew import InstrumentArrayConverter
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt, arange, ones, zeros


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
        out = IntegratePeaksSkew(
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

    def test_peak_mask_validation_with_ncol_max(self):
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

    def test_nrows_edge_ncols_edge_in_array_converter(self):
        array_converter = InstrumentArrayConverter(self.ws)
        ipk = 0
        pk = self.peaks.getPeak(ipk)
        detid = self.peaks.column("DetID")[ipk]
        bank = self.peaks.column("BankName")[ipk]
        for nrows_edge in range(1, 3):
            for ncols_edge in range(1, 3):
                peak_data = array_converter.get_peak_data(pk, detid, bank, nrows=7, ncols=7, nrows_edge=nrows_edge, ncols_edge=ncols_edge)
                self.assertTrue(peak_data.det_edges[:nrows_edge, :].all())
                self.assertTrue(peak_data.det_edges[-nrows_edge:, :].all())
                self.assertFalse(peak_data.det_edges[nrows_edge:-nrows_edge:, ncols_edge:-ncols_edge].any())
        self.assertTrue((peak_data.detids == arange(25, 50).reshape(5, 5).T).all())

    def test_array_converter_finds_adjacent_banks_to_left_for_component_array_detectors(self):
        array_converter = InstrumentArrayConverter(self.ws_comp_arr)
        ipk = 3  # first tube in bank 9 (adjacent to last tube of bank 10)
        pk = self.peaks_comp_arr.getPeak(ipk)
        detid = self.peaks_comp_arr.column("DetID")[ipk]
        bank = self.peaks_comp_arr.column("BankName")[ipk]
        peak_data = array_converter.get_peak_data(pk, detid, bank, nrows=5, ncols=5, nrows_edge=1, ncols_edge=1)
        self.assertFalse(peak_data.det_edges.any())  # no detector edges as found tubes in adjacent bank
        self.assertTrue(
            (
                peak_data.detids
                == array(
                    [
                        [10101253, 10100253, 9707253, 9706253, 9705253],
                        [10101254, 10100254, 9707254, 9706254, 9705254],
                        [10101255, 10100255, 9707255, 9706255, 9705255],
                        [10101256, 10100256, 9707256, 9706256, 9705256],
                        [10101257, 10100257, 9707257, 9706257, 9705257],
                    ]
                )
            ).all()
        )
        self.assertEqual(peak_data.irow, 2)
        self.assertEqual(peak_data.icol, 2)

    def test_array_converter_finds_adjacent_banks_to_right_for_component_array_detectors(self):
        array_converter = InstrumentArrayConverter(self.ws_comp_arr)
        ipk = 2  # last tube bank 10 (adjacent to first tube of bank 9)
        pk = self.peaks_comp_arr.getPeak(ipk)
        detid = self.peaks_comp_arr.column("DetID")[ipk]
        bank = self.peaks_comp_arr.column("BankName")[ipk]
        peak_data = array_converter.get_peak_data(pk, detid, bank, nrows=5, ncols=5, nrows_edge=1, ncols_edge=1)
        self.assertFalse(peak_data.det_edges.any())  # no detector edges as found tubes in adjacent bank
        self.assertTrue(
            (
                peak_data.detids
                == array(
                    [
                        [10102253, 10101253, 10100253, 9707253, 9706253],
                        [10102254, 10101254, 10100254, 9707254, 9706254],
                        [10102255, 10101255, 10100255, 9707255, 9706255],
                        [10102256, 10101256, 10100256, 9707256, 9706256],
                        [10102257, 10101257, 10100257, 9707257, 9706257],
                    ]
                )
            ).all()
        )
        self.assertEqual(peak_data.irow, 2)
        self.assertEqual(peak_data.icol, 2)

    def test_nrows_edge_ncols_edge_in_array_converter_component_array(self):
        array_converter = InstrumentArrayConverter(self.ws_comp_arr)
        # top and bottom of first tube in bank 10 (no adjacent bank on LHS)
        for ipk in range(2):
            pk = self.peaks_comp_arr.getPeak(ipk)
            detid = self.peaks_comp_arr.column("DetID")[ipk]
            bank = self.peaks_comp_arr.column("BankName")[ipk]
            peak_data = array_converter.get_peak_data(pk, detid, bank, nrows=7, ncols=5, nrows_edge=3, ncols_edge=2)
            irow_expected, icol_expected = (0, 0) if ipk == 0 else (3, 0)
            self.assertEqual(peak_data.irow, irow_expected)
            self.assertEqual(peak_data.icol, icol_expected)
            det_edges_expected = ones((4, 3), dtype=bool)
            if ipk == 0:
                det_edges_expected[-1, -1] = False
            else:
                det_edges_expected[0, -1] = False
            self.assertTrue((peak_data.det_edges == det_edges_expected).all())
        # middle of 5th tube from end of bank 5 (no adjacent bank on RHS)
        ipk = 4
        pk = self.peaks_comp_arr.getPeak(ipk)
        detid = self.peaks_comp_arr.column("DetID")[ipk]
        bank = self.peaks_comp_arr.column("BankName")[ipk]
        peak_data = array_converter.get_peak_data(pk, detid, bank, nrows=5, ncols=7, nrows_edge=1, ncols_edge=2)
        self.assertEqual(peak_data.irow, 2)
        self.assertEqual(peak_data.icol, 3)
        det_edges_expected = zeros((5, 7), dtype=bool)
        det_edges_expected[:, -1] = True  # last tube in window is second from end of bank and ncols_edge=2
        self.assertTrue((peak_data.det_edges == det_edges_expected).all())


if __name__ == "__main__":
    unittest.main()
