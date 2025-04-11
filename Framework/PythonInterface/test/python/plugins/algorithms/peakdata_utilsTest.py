# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    CreatePeaksWorkspace,
    AddPeak,
    AnalysisDataService,
    LoadEmptyInstrument,
    SetInstrumentParameter,
)
from plugins.algorithms.peakdata_utils import InstrumentArrayConverter
from testhelpers import WorkspaceCreationHelper
from numpy import array, sqrt, arange, ones, zeros


class peakdata_utilsTest(unittest.TestCase):
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

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

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
