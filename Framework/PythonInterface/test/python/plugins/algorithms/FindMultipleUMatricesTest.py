import unittest

from mantid.simpleapi import (
    FindMultipleUMatrices,
    AnalysisDataService,
    IndexPeaks,
    SetUB,
    CreatePeaksWorkspace,
    LoadEmptyInstrument,
    FilterPeaks,
    CloneWorkspace,
    DeleteTableRows,
)
from mantid.kernel import V3D
import numpy as np
from scipy.spatial.transform import Rotation as rot


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
        cls.ubs = [np.diag([1 / cls.alatt["a"], 1 / cls.alatt["b"], 1 / cls.alatt["c"]])]  # produces 7 peaks
        cls.ubs.append(rot.from_rotvec([0, 0, 90], degrees=True).as_matrix() @ cls.ubs[0])  # produces 5 peaks
        cls.peaks = CreatePeaksWorkspace(InstrumentWorkspace=cls.ws, NumberOfPeaks=0, OutputWorkspace=f"peaks")
        for iub, ub in enumerate(cls.ubs):
            SetUB(cls.peaks, UB=ub)
            add_peaksHKL(cls.peaks, range(1, 3), range(1, 3), range(1, 4), iub)
        # dict of default kwargs for FindMultipleUMatrices
        cls.default_kwargs = {
            **cls.alatt,
            "MinDSpacing": 1.75,
            "MaxDSpacing": 2.5,
            "HKLTolerance": 0.15,
            "DSpacingTolerance": 0.1,
            "AngleTolerance": 1,
            "Spacegroup": "P m m m",
            "OptimiseFoundUBs": True,
            "NumberOfUBs": 2,
        }

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def _assert_correct_ubs_found(self, peaks_grp_ws, found_peaks, **kwargs):
        kwargs = {"Tolerance": 0.15, **kwargs}
        nindexed_tot = 0
        for ipk, pks in enumerate(peaks_grp_ws):
            SetUB(found_peaks, UB=pks.sample().getOrientedLattice().getUB(), EnableLogging=False, StoreInADS=False)
            nindexed, *_ = IndexPeaks(found_peaks, EnableLogging=False, StoreInADS=False, **kwargs)
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

    def test_respects_spacegroup(self):
        kwargs = {**self.default_kwargs, "Spacegroup": "F m m m"}  # copy with this field overwritten

        with self.assertRaisesRegex(RuntimeError, "No valid UBs found using 7 peaks in d-spacing range."):
            FindMultipleUMatrices(PeaksWorkspace=self.peaks, OutputWorkspace="peaks_out2", **kwargs)

    def test_respects_min_and_max_dspacing(self):
        kwargs = {**self.default_kwargs, "MinDSpacing": 11, "MaxDSpacing": 12}  # copy with this field overwritten

        with self.assertRaisesRegex(RuntimeError, "No valid UBs found using 0 peaks in d-spacing range."):
            FindMultipleUMatrices(PeaksWorkspace=self.peaks, OutputWorkspace="peaks_out3", **kwargs)

    def test_min_angle_between_ub(self):
        kwargs = {**self.default_kwargs, "MinAngleBetweenUB": 100}  # copy with this field overwritten

        peaks_out = FindMultipleUMatrices(PeaksWorkspace=self.peaks, OutputWorkspace="peaks_out4", **kwargs)

        self.assertEqual(peaks_out.getNumberOfEntries(), 1)  # UBs treated as similar - keep one indexing most peaks
        # test finds UB corresponding to first UB (as produces 7 peaks vs 5 peaks for other UB)
        found_peaks = FilterPeaks(
            InputWorkspace=self.peaks, StoreInADS=False, FilterVariable="RunNumber", FilterValue=0, Operator="=", EnableLogging=False
        )
        self._assert_correct_ubs_found(peaks_out, found_peaks)

    def test_min_num_peaks_indexed_respected(self):
        peaks_subset = CloneWorkspace(InputWorkspace=self.peaks, OutputWorkspace="peaks_subset")
        DeleteTableRows(TableWorkspace=peaks_subset, Rows=range(4, peaks_subset.getNumberPeaks()))

        with self.assertRaisesRegex(RuntimeError, "There must be at least 3 peaks for each UB requested."):
            FindMultipleUMatrices(PeaksWorkspace=peaks_subset, OutputWorkspace="peaks_out5", **self.default_kwargs)

    def test_mod_vectors(self):
        mod_vec = [0.35, 0, 0.5]
        mod_vec_kwargs = {"MaxOrder": 1, "ModVector1": ",".join([str(v) for v in mod_vec])}
        kwargs = {**self.default_kwargs, "NumberOfUBs": 1, "MinDSpacing": 1.25, "MaxDSpacing": 3.5, **mod_vec_kwargs}
        peaks_modvec = CloneWorkspace(InputWorkspace=self.peaks, OutputWorkspace=f"peaks_modvec")
        # add satellite peaks using ubs[1]
        SetUB(peaks_modvec, UB=self.ubs[1])
        for ipk in range(1, 5):
            main_pk = self.peaks.getPeak(self.peaks.getNumberPeaks() - ipk)
            pk = peaks_modvec.createPeakHKL(main_pk.getIntHKL() + V3D(*mod_vec))
            if pk.getDetectorID() > 0:
                pk.setRunNumber(main_pk.getRunNumber())
                peaks_modvec.addPeak(pk)

        peaks_out = FindMultipleUMatrices(PeaksWorkspace=peaks_modvec, OutputWorkspace="peaks_out6", **kwargs)

        # get peaks from final UB (now has more peaks than first UB - so is returned as best UB)
        found_peaks = FilterPeaks(
            InputWorkspace=self.peaks, StoreInADS=False, FilterVariable="RunNumber", FilterValue=1, Operator="=", EnableLogging=False
        )
        self._assert_correct_ubs_found(peaks_out, found_peaks, **mod_vec_kwargs)


if __name__ == "__main__":
    unittest.main()
