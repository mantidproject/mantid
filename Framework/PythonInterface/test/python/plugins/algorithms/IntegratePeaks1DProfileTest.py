# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.simpleapi import (
    IntegratePeaks1DProfile,
    CreatePeaksWorkspace,
    AddPeak,
    Rebin,
    LoadEmptyInstrument,
    AnalysisDataService,
    SortPeaksWorkspace,
    CloneWorkspace,
    BackToBackExponential,
    DeleteTableRows,
)
import numpy as np
import tempfile
import shutil
from os import path


class IntegratePeaks1DProfileTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument with RectangularDetector banks and create a peak table
        cls.ws = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="SXD")
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")

        # rebin to get required blocksize
        cls.ws = Rebin(InputWorkspace=cls.ws, OutputWorkspace=cls.ws.name(), Params="9900,10,10800", PreserveEvents=False)
        cls.ws += 50  # add constant background

        cls.peaks_edge = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace="peaks_incl_edge")
        cls.detids = [6049, 4220]
        ispecs = cls.ws.getIndicesFromDetectorIDs(cls.detids)
        # simulate peak
        cls.pk_tof = 10250
        pk_func = BackToBackExponential(I=1e4, X0=cls.pk_tof)
        for ipk, detid in enumerate(cls.detids):
            AddPeak(PeaksWorkspace=cls.peaks_edge, RunWorkspace=cls.ws, TOF=cls.pk_tof, DetectorID=detid)
            pk_func["X0"] = cls.pk_tof
            pk_func.function.setMatrixWorkspace(cls.ws, ispecs[ipk], 0.0, 0.0)
            for ipix, ispec in enumerate(np.arange(ispecs[ipk], ispecs[ipk] + 2)):
                if ipix > 0:
                    # add small shift to simulated peak in second pixel so can check d-spacing tolerance
                    pk_func["X0"] = cls.pk_tof + 5
                y = cls.ws.dataY(int(ispec))
                y += pk_func(cls.ws.readX(int(ispec))[:-1])  # note shifts peak centre by half a bin
                cls.ws.setE(int(ispec), np.sqrt(y))

        # clone workspace and delete edge peak so only 1 peak (quicker for most tests)
        cls.peaks = CloneWorkspace(cls.peaks_edge, OutputWorkspace="peaks")
        DeleteTableRows(cls.peaks, Rows=[1])

        # default IntegratePeaks1DProfile kwargs
        cls.profile_kwargs = {
            "GetNBinsFromBackToBackParams": True,
            "NFWHM": 6,
            "CostFunction": "RSq",
            "FixPeakParameters": "A",
            "FractionalChangeDSpacing": 0.025,
            "IntegrateIfOnEdge": True,
            "LorentzCorrection": False,
            "IOverSigmaThreshold": 1,
            "NRowsEdge": 2,
            "NColsEdge": 2,
        }

        # output file dir
        cls._test_dir = tempfile.mkdtemp()
        cls.default_intens_over_sigma = 30.966

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

    def test_exec_IntegrateIfOnEdge_True(self):
        out = IntegratePeaks1DProfile(
            InputWorkspace=self.ws, PeaksWorkspace=self.peaks_edge, OutputWorkspace="peaks_int_1", **self.profile_kwargs
        )
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], self.default_intens_over_sigma, delta=1e-1)
        self.assertAlmostEqual(out.column("Intens/SigInt")[1], self.default_intens_over_sigma, delta=1e-1)

    def test_exec_IntegrateIfOnEdge_False(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["IntegrateIfOnEdge"] = False
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks_edge, OutputWorkspace="peaks_int_2", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], self.default_intens_over_sigma, delta=1e-1)
        self.assertAlmostEqual(out.column("Intens/SigInt")[1], 0.0, delta=1e-2)

    def test_exec_IntegrateIfOnEdge_False_respects_detector_masking(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["IntegrateIfOnEdge"] = False
        kwargs["NRowsEdge"] = 0
        kwargs["NColsEdge"] = 0
        ws_masked = CloneWorkspace(InputWorkspace=self.ws)
        det_info = ws_masked.detectorInfo()
        det_info.setMasked(det_info.indexOf(self.peaks_edge.getPeak(1).getDetectorID()), True)

        out = IntegratePeaks1DProfile(
            InputWorkspace=ws_masked, PeaksWorkspace=self.peaks_edge, OutputWorkspace="peaks_int_2_masked", **kwargs
        )
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], self.default_intens_over_sigma, delta=1e-1)
        self.assertAlmostEqual(out.column("Intens/SigInt")[1], 0.0, delta=1e-2)

    def test_exec_poisson_cost_func(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["CostFunction"] = "Poisson"
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_3", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 30.95, delta=1e-2)

    def test_exec_chisq_cost_func(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["CostFunction"] = "ChiSq"
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_4", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], self.default_intens_over_sigma, delta=1e-2)

    def test_exec_hessian_error_strategy(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["ErrorStrategy"] = "Hessian"
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_5", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 62502473, delta=1)  # not realistic fit - no noise!

    def test_exec_IOverSigmaThreshold_respected(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["IOverSigmaThreshold"] = 100  # set this higher than I/sigma in any pixel i.e. should not fit any pixels
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_6", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 0.0, delta=1e-2)

    def test_exec_gaussian_peak_func(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["PeakFunction"] = "Gaussian"
        kwargs["FixPeakParameters"] = ""
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_7", **kwargs)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 31.04, delta=1e-2)

    def test_exec_OutputFile(self):
        out_file = path.join(self._test_dir, "out.pdf")
        IntegratePeaks1DProfile(
            InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_8", OutputFile=out_file, **self.profile_kwargs
        )
        # check output file saved
        self.assertTrue(path.exists(out_file))

    def test_exec_FractionalChangeDSpacing(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["FractionalChangeDSpacing"] = 1e-10
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_9", **kwargs)
        # I/sigma different now d-spacing constrained
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 31.16, delta=1e-2)

    def test_exec_fix_peak_params(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["FixPeakParameters"] = ["I"]
        out = IntegratePeaks1DProfile(InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_9", **kwargs)
        # I/sig slightly different as fixed intensity at initial guess (pretty good guess!)
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 30.964, delta=1e-3)

    def test_exec_NPixMin_respected(self):
        kwargs = self.profile_kwargs.copy()
        kwargs["NPixMin"] = 3  # only 2 pixels in simulated data
        out = IntegratePeaks1DProfile(
            InputWorkspace=self.ws, PeaksWorkspace=self.peaks, OutputWorkspace="peaks_int_10", **kwargs
        )
        self.assertAlmostEqual(out.column("Intens/SigInt")[0], 0.0, delta=1e-2)


if __name__ == "__main__":
    unittest.main()
