# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from types import SimpleNamespace
from unittest import mock

from mantid.simpleapi import (
    RefineSingleCrystalGoniometer,
    CloneWorkspace,
    LoadIsawPeaks,
    FindUBUsingIndexedPeaks,
    FindUBUsingFFT,
    IndexPeaks,
)
import plugins.algorithms.RefineSingleCrystalGoniometer as refine_goniometer_module


class FakeMtd:
    def __init__(self):
        self.workspaces = set()

    def __contains__(self, name):
        return name in self.workspaces


class FakePeaks:
    def name(self):
        return "peaks"


class RefineSingleCrystalGoniometerTest(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "TOPAZ_2479.peaks"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

        FindUBUsingIndexedPeaks(PeaksWorkspace="peaks", Tolerance=0.12)
        CloneWorkspace(InputWorkspace="peaks", OutputWorkspace="refined")

        index_null = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        initial = index_null.NumIndexed

        RefineSingleCrystalGoniometer(Peaks="refined", Tolerance=0.12, Cell="Triclinic", NumIterations=1)

        index_refine = IndexPeaks(PeaksWorkspace="refined", Tolerance=0.12)

        final = index_refine.NumIndexed

        assert final > initial


class RefineSingleCrystalGoniometerCleanupTest(unittest.TestCase):
    def test_deletes_temp_workspace_when_fft_indexing_fails(self):
        fake_mtd = FakeMtd()

        def filter_peaks(**kwargs):
            fake_mtd.workspaces.add(kwargs["OutputWorkspace"])

        def delete_workspace(**kwargs):
            fake_mtd.workspaces.remove(kwargs["Workspace"])

        properties = {
            "Tolerance": 0.12,
            "MinD": 5.0,
            "MaxD": 15.0,
            "LatticeOutlierTolerance": 5.0,
        }

        def get_property(_, name):
            return SimpleNamespace(value=properties[name])

        alg = refine_goniometer_module.RefineSingleCrystalGoniometer()

        with (
            mock.patch.object(refine_goniometer_module.RefineSingleCrystalGoniometer, "getProperty", get_property),
            mock.patch.object(refine_goniometer_module, "mtd", fake_mtd),
            mock.patch.object(refine_goniometer_module, "FilterPeaks", side_effect=filter_peaks),
            mock.patch.object(refine_goniometer_module, "FindUBUsingFFT", side_effect=RuntimeError("fft failed")),
            mock.patch.object(refine_goniometer_module, "DeleteWorkspace", side_effect=delete_workspace) as delete_workspace_mock,
        ):
            with self.assertRaisesRegex(RuntimeError, "fft failed"):
                alg._index_runs_using_fft(FakePeaks(), [1])

        delete_workspace_mock.assert_called_once_with(Workspace="_fft_tmp_1")
        self.assertNotIn("_fft_tmp_1", fake_mtd)


class RefineSingleCrystalGoniometerTestWithLargeOffset(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "TOPAZ_293K_Triclinic_P_unreliable_motors.nxs"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

        # IndexPeaks requires an OrientedLattice to already be set, so seed
        # one with a single FindUBUsingFFT across all runs. With unreliable
        # per-run goniometer offsets, this single shared UB indexes poorly.
        FindUBUsingFFT(PeaksWorkspace="peaks", MinD=5, MaxD=15)

        index_null = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        initial = index_null.NumIndexed

        # LargeOffset=True indexes each run independently via FindUBUsingFFT
        # (robust to the large per-run misorientation) before refining the
        # UB and goniometer offsets jointly.
        RefineSingleCrystalGoniometer(
            Peaks="peaks",
            Tolerance=0.12,
            Cell="Triclinic",
            NumIterations=8,
            LargeOffset=True,
            MinD=5,
            MaxD=15,
        )

        index_refine = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        final = index_refine.NumIndexed

        assert final > initial


if __name__ == "__main__":
    unittest.main()
