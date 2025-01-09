# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.simpleapi import (
    IntegratePeaksShoeboxTOF,
    CreatePeaksWorkspace,
    AddPeak,
    LoadParameterFile,
    AnalysisDataService,
    SortPeaksWorkspace,
    CloneWorkspace,
)
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt
import tempfile
import shutil
from os import path
import json

XML_PARAMS = """
<?xml version="1.0" encoding="UTF-8" ?>
<parameter-file instrument = "basic_rect" valid-from = "2013-11-06T00:00:00">
<component-link name = "bank1">
<parameter name="BackToBackExponential:A" type="fitting">
<formula eq="2" unit="TOF" result-unit="1/TOF" /> <fixed />
</parameter>
<parameter name="BackToBackExponential:B" type="fitting">
<formula eq="2" unit="TOF" result-unit="1/TOF" /> <fixed />
</parameter>
<parameter name="BackToBackExponential:S" type="fitting">
<formula eq="0.1" unit="TOF" result-unit="TOF" />
</parameter>
</component-link>
</parameter-file>
"""


class IntegratePeaksShoeboxTOFTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument with RectangularDetector banks and create a peak table
        cls.ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(1, 7, 13)  # nbanks, npix, nbins
        AnalysisDataService.addOrReplace("ws_rect", cls.ws)
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        # fake peak centred on ispec=30 (detid=79) and TOF=5 - near middle of bank
        peak_1D = array([0, 0, 4, 6, 4, 0, 0, 0, 0, 0, 0, 0, 0])
        cls.ws.setY(30, cls.ws.readY(30) + peak_1D)
        for ispec in [23, 29, 30, 31, 37]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + peak_1D)
        # fake peak centred on ispec=12 (detid=61) and TOF=7 - near detector edge
        cls.ws.setY(12, cls.ws.readY(12) + peak_1D[::-1])
        for ispec in [5, 11, 12, 13, 19]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + peak_1D[::-1])
        # add background and set errors
        for ispec in range(cls.ws.getNumberHistograms()):
            cls.ws.setY(ispec, cls.ws.readY(ispec) + 0.5)
            cls.ws.setE(ispec, sqrt(cls.ws.readY(ispec)))
        # add peaks
        cls.peaks = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace="peaks")
        AddPeak(PeaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=5, DetectorID=85)
        AddPeak(PeaksWorkspace=cls.peaks, RunWorkspace=cls.ws, TOF=9, DetectorID=69)
        [cls.peaks.getPeak(ipk).setHKL(ipk, ipk, ipk) for ipk in range(cls.peaks.getNumberPeaks())]
        # Add back-to-back exponential params
        LoadParameterFile(cls.ws, ParameterXML=XML_PARAMS)
        # output file dir
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

    def _assert_found_correct_peaks(self, peak_ws, i_over_sigs=[6.4407, 4.0207]):
        self.assertEqual(peak_ws.getNumberPeaks(), 2)
        peak_ws = SortPeaksWorkspace(
            InputWorkspace=peak_ws, OutputWorkspace=peak_ws.name(), ColumnNameToSortBy="DetID", SortAscending=False
        )
        pk = peak_ws.getPeak(0)
        self.assertEqual(pk.getDetectorID(), 85)
        self.assertAlmostEqual(pk.getTOF(), 5.0, delta=1e-8)
        self.assertAlmostEqual(pk.getIntensityOverSigma(), i_over_sigs[0], delta=1e-4)
        pk = peak_ws.getPeak(1)
        self.assertEqual(pk.getDetectorID(), 69)
        self.assertAlmostEqual(pk.getTOF(), 9.0, delta=1e-8)
        self.assertAlmostEqual(pk.getIntensityOverSigma(), i_over_sigs[1], delta=1e-4)

    def test_exec_IntegrateIfOnEdge_False(self):
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks1",
            GetNBinsFromBackToBackParams=False,
            NRows=3,
            NCols=3,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=False,
        )
        # check edge peak integrated but lower I/sigma as shell would overlap edge
        self._assert_found_correct_peaks(out, 2 * [0.0])

    def test_exec_IntegrateIfOnEdge_False_respects_detector_masking(self):
        ws_masked = CloneWorkspace(InputWorkspace=self.ws)
        det_info = ws_masked.detectorInfo()
        [det_info.setMasked(det_info.indexOf(pk.getDetectorID()), True) for pk in self.peaks]

        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=ws_masked,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks1_masked",
            GetNBinsFromBackToBackParams=False,
            NRows=3,
            NCols=3,
            NBins=3,
            NRowsEdge=0,
            NColsEdge=0,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=False,
        )
        # check edge peak integrated but lower I/sigma as shell would overlap edge
        self._assert_found_correct_peaks(out, 2 * [0.0])

    def test_exec_IntegrateIfOnEdge_True(self):
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks2",
            GetNBinsFromBackToBackParams=False,
            NRows=3,
            NCols=3,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=True,
        )
        # check edge peaks integrated but lower I/sigma on second peak as kernel overlaps detector edge
        self._assert_found_correct_peaks(out)

    def test_exec_GetNBinsFromBackToBackParams(self):
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks3",
            GetNBinsFromBackToBackParams=True,
            NRows=3,
            NCols=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=True,
        )
        # check edge peaks integrated but lower I/sigma on second peak as kernel overlaps detector edge
        self._assert_found_correct_peaks(out)

    def test_exec_OptimiseShoebox(self):
        # make kernel larger than optimum
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks4",
            GetNBinsFromBackToBackParams=False,
            NRows=5,
            NCols=5,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=True,
            IntegrateIfOnEdge=True,
        )
        # check I/sigma first peak unchanged, second peak improved!
        self._assert_found_correct_peaks(out, i_over_sigs=[6.4407, 4.0207])

    def test_exec_OptimiseShoebox_respects_WeakPeakThreshold(self):
        # make kernel larger than optimum
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks5",
            GetNBinsFromBackToBackParams=False,
            NRows=5,
            NCols=5,
            NBins=3,
            WeakPeakThreshold=10.0,
            OptimiseShoebox=True,
            IntegrateIfOnEdge=True,
        )
        # check I/sigmas much worse if not optimised
        self._assert_found_correct_peaks(out, i_over_sigs=[4.4631, 2.3966])

    def test_exec_OutputFile(self):
        out_file = path.join(self._test_dir, "out.pdf")
        IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks1",
            GetNBinsFromBackToBackParams=False,
            NRows=3,
            NCols=3,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=False,
            OutputFile=out_file,
        )
        # check output file saved
        self.assertTrue(path.exists(out_file))

    @mock.patch("IntegratePeaksShoeboxTOF.find_nearest_peak_in_data_window")
    def test_exec_no_peak(self, mock_find_ipos):
        mock_find_ipos.return_value = None
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks6",
            GetNBinsFromBackToBackParams=True,
            NRows=3,
            NCols=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=True,
        )
        self._assert_found_correct_peaks(out, i_over_sigs=2 * [0.0])

    def test_exec_peak_shape_when_IntegrateIfOnEdge_False(self):
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks1",
            GetNBinsFromBackToBackParams=False,
            NRows=3,
            NCols=3,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=False,
            IntegrateIfOnEdge=False,
        )
        self.assertEqual(out.getNumberPeaks(), 2)
        self.assertEqual(out.getPeak(0).getPeakShape().shapeName(), "none")
        self.assertEqual(out.getPeak(1).getPeakShape().shapeName(), "none")

    def test_exec_OptimiseShoebox_peak_shape(self):
        # make kernel larger than optimum
        out = IntegratePeaksShoeboxTOF(
            InputWorkspace=self.ws,
            PeaksWorkspace=self.peaks,
            OutputWorkspace="peaks4",
            GetNBinsFromBackToBackParams=False,
            NRows=5,
            NCols=5,
            NBins=3,
            WeakPeakThreshold=0.0,
            OptimiseShoebox=True,
            IntegrateIfOnEdge=True,
        )

        self.assertEqual(out.getNumberPeaks(), 2)

        def _test_shapes(peaksws, start_end_points):
            for i_pk, pk in enumerate(peaksws):
                self.assertEqual(pk.getPeakShape().shapeName(), "detectorbin")
                pk_shape_dict = json.loads(pk.getPeakShape().toJSON())
                self.assertEqual(len(pk_shape_dict["detectors"]), 9)
                self.assertEqual(pk_shape_dict["algorithm_name"], "IntegratePeaksShoeboxTOF")
                for det in pk_shape_dict["detectors"]:
                    self.assertEqual(det["startX"], start_end_points[i_pk][0])
                    self.assertEqual(det["endX"], start_end_points[i_pk][1])

        _test_shapes(out, ((2.5, 4.5), (8.5, 10.5)))


if __name__ == "__main__":
    unittest.main()
