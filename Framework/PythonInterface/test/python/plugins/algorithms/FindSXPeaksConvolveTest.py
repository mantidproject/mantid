# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    FindSXPeaksConvolve,
    LoadParameterFile,
    AnalysisDataService,
)
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt


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


class FindSXPeaksConvolveTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument with RectangularDetector banks and create a peak table
        cls.ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(1, 7, 13)  # nbanks, npix, nbins
        AnalysisDataService.addOrReplace("ws_rect", cls.ws)
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        # fake peak centred on ispec=25 (detid=74) and TOF=5 - near middle of bank
        peak_1D = array([0, 0, 0, 0, 8, 12, 8, 0, 0, 0, 0, 0, 0])
        cls.ws.setY(25, cls.ws.readY(30) + peak_1D)
        for ispec in [18, 24, 25, 26, 32]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + peak_1D)
            cls.ws.setE(ispec, sqrt(cls.ws.readY(ispec)))
        # fake peak centred on ispec=12 (detid=61) and TOF=7 - near detector edge
        cls.ws.setY(12, cls.ws.readY(12) + peak_1D[::-1])
        for ispec in [5, 11, 12, 13, 19]:
            cls.ws.setY(ispec, cls.ws.readY(ispec) + peak_1D[::-1])
            cls.ws.setE(ispec, sqrt(cls.ws.readY(ispec)))
        # Add back-to-back exponential params
        LoadParameterFile(cls.ws, ParameterXML=XML_PARAMS)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def _assert_found_correct_peaks(self, peak_ws, integrated=True):
        self.assertEqual(peak_ws.getNumberPeaks(), 1)
        pk = peak_ws.getPeak(0)
        self.assertEqual(pk.getDetectorID(), 74)
        if integrated:
            self.assertAlmostEqual(pk.getTOF(), 5.0, delta=1e-8)
            self.assertAlmostEqual(pk.getIntensityOverSigma(), 7.4826, delta=1e-4)

    def test_exec_specify_nbins(self):
        out = FindSXPeaksConvolve(
            InputWorkspace=self.ws, PeaksWorkspace="peaks1", NRows=3, NCols=3, NBins=3, ThresholdIoverSigma=3.0, MinFracSize=0.02
        )
        self._assert_found_correct_peaks(out)

    def test_exec_get_nbins_from_back_to_back_params(self):
        out = FindSXPeaksConvolve(
            InputWorkspace=self.ws,
            PeaksWorkspace="peaks1",
            NRows=3,
            NCols=3,
            GetNBinsFromBackToBackParams=True,
            ThresholdIoverSigma=3.0,
            MinFracSize=0.02,
        )
        self._assert_found_correct_peaks(out)

    def test_exec_IoverSigma_threshold(self):
        out = FindSXPeaksConvolve(InputWorkspace=self.ws, PeaksWorkspace="peaks3", NRows=3, NCols=3, NBins=3, ThresholdIoverSigma=100.0)
        self.assertEqual(out.getNumberPeaks(), 0)

    def test_exec_min_frac_size(self):
        out = FindSXPeaksConvolve(
            InputWorkspace=self.ws, PeaksWorkspace="peaks4", NRows=3, NCols=3, NBins=5, ThresholdIoverSigma=3.0, MinFracSize=0.5
        )
        self.assertEqual(out.getNumberPeaks(), 0)

    def test_exec_VarOverMean(self):
        out = FindSXPeaksConvolve(
            InputWorkspace=self.ws,
            PeaksWorkspace="peaks5",
            NRows=5,
            NCols=5,
            Nbins=3,
            PeakFindingStrategy="VarianceOverMean",
            ThresholdVarianceOverMean=3.0,
            MinFracSize=0.02,
        )
        self._assert_found_correct_peaks(out, integrated=False)


if __name__ == "__main__":
    unittest.main()
