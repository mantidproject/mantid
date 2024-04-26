import unittest

# from testhelpers import create_algorithm
from mantid.simpleapi import (
    FindMultipleUMatrices,
    AnalysisDataService,
    IndexPeaks,
    SetUB,
    CreatePeaksWorkspace,
    LoadEmptyInstrument,
    FilterPeaks,
)
import numpy as np
from FindGoniometerFromUB import getR


def add_peaksHKL(peaks, Hs, Ks, Ls, iub):
    for h in Hs:
        for k in Ks:
            for l in Ls:
                pk = peaks.createPeakHKL([h, k, l])
                if pk.getDetectorID() > 0:
                    pk.setRunNumber(iub)  # so can keep track of UB
                    peaks.addPeak(pk)


class FindMultipleUMatricesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load empty instrument
        cls.ws = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="empty_SXD")
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        # create a peak tables of orthorhombic domains with lattice parameters a=4, b=5, c=10
        cls.alatt = {"a": 4, "b": 5, "c": 10, "alpha": 90, "beta": 90, "gamma": 90}
        ubs = [
            np.diag([1 / cls.alatt["a"], 1 / cls.alatt["b"], 1 / cls.alatt["c"]]),  # produces 7 peaks
            np.diag([1 / cls.alatt["b"], 1 / cls.alatt["a"], 1 / cls.alatt["c"]]),
        ]  # produces 5 peaks
        cls.peaks = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace=f"peaks")
        for iub, ub in enumerate(ubs):
            SetUB(cls.peaks, UB=ub)
            add_peaksHKL(cls.peaks, range(1, 3), range(1, 3), range(1, 4), iub)
        # dict of default kwargs for FindMultipleUMatrices
        cls.default_kwargs = {
            **cls.alatt,
            "MinDSpacing": 1.75,
            "MaxDSpacing": 3.5,
            "HKLTolerance": 0.15,
            "DSpacingTolerance": 0.1,
            "AngleTolerance": 1,
            "Spacegroup": "P 21 21 21",
            "OptimiseFoundUBs": True,
            "NumberOfUBs": 2,
        }

    @classmethod
    def tearDownClass(self):
        AnalysisDataService.clear()

    def _assert_correct_ubs_found(self, peaks_grp_ws, found_peaks):
        nindexed_tot = 0
        for ipk, pks in enumerate(peaks_grp_ws):
            SetUB(found_peaks, UB=pks.sample().getOrientedLattice().getUB(), EnableLogging=False, StoreInADS=False)
            nindexed, *_ = IndexPeaks(found_peaks, Tolerance=0.15, EnableLogging=False, StoreInADS=False)
            nindexed_tot += nindexed
            peaks_tmp = FilterPeaks(
                InputWorkspace=found_peaks, StoreInADS=False, FilterVariable="h^2+k^2+l^2", FilterValue=0, Operator=">", EnableLogging=False
            )
            # check all peaks indexed with this UB are consistent with the same UB used to create them
            self.assertEqual(len(set(peaks_tmp.column("RunNumber"))), 1)
        self.assertEqual(nindexed_tot, found_peaks.getNumberPeaks())  # all peaks indexed by the UBs found

    def test_finds_two_valid_ubs(self):
        peaks_out = FindMultipleUMatrices(PeaksWorkspace=self.peaks, OutputWorkspace="peaks_out0", **self.default_kwargs)

        self.assertEqual(peaks_out.getNumberOfEntries(), 2)
        self._assert_correct_ubs_found(peaks_out, self.peaks)

    def test_finds_one_ub_that_indexes_most_peaks(self):
        kwargs = {**self.default_kwargs, "NumberOfUBs": 1}  # copy with this field overwritten

        peaks_out = FindMultipleUMatrices(PeaksWorkspace=self.peaks, OutputWorkspace="peaks_out1", **kwargs)

        self.assertEqual(peaks_out.getNumberOfEntries(), 1)
        # test finds UB corresponding to first UB (as produces 7 peaks vs 5 peaks for other UB)
        found_peaks = FilterPeaks(
            InputWorkspace=self.peaks, StoreInADS=False, FilterVariable="RunNumber", FilterValue=0, Operator="=", EnableLogging=False
        )
        self._assert_correct_ubs_found(peaks_out, found_peaks)


if __name__ == "__main__":
    unittest.main()
