# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods
import numpy as np
import systemtesting
from mantid.simpleapi import (
    HB3AAdjustSampleNorm,
    HB3AFindPeaks,
    HB3AIntegratePeaks,
    HB3APredictPeaks,
    mtd,
    DeleteWorkspaces,
    CreateMDWorkspace,
    LoadEmptyInstrument,
    AddTimeSeriesLog,
    SetUB,
    FakeMDEventData,
    FindPeaksMD,
    ShowPossibleCells,
    FindUBUsingFFT,
    OptimizeLatticeForCellType,
    SelectCellOfType,
    HasUB,
)
from mantid.kernel import V3D
from mantid.geometry import OrientedLattice, SpaceGroupFactory


class SingleFileFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "SingleFileFindPeaksIntegrate"
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data")

        HB3AFindPeaks(InputWorkspace=ws_name + "_data", CellType="Orthorhombic", Centering="F", OutputWorkspace=ws_name + "_peaks")

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        peaks = mtd[ws_name + "_integrated"]

        self.assertEqual(peaks.getNumberPeaks(), 15)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.231258554, 1.0e-7)
        self.assertDelta(ol.b(), 5.257701834, 1.0e-7)
        self.assertDelta(ol.c(), 19.67041036, 1.0e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_peaks", ws_name + "_integrated"])


class SingleFilePredictPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "SingleFilePredictPeaksIntegrate"
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data")

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", OutputWorkspace=ws_name + "_peaks")

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        peaks = mtd[ws_name + "_integrated"]

        self.assertEqual(peaks.getNumberPeaks(), 112)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.238389802, 1.0e-7)
        self.assertDelta(ol.b(), 5.238415883, 1.0e-7)
        self.assertDelta(ol.c(), 19.65194598, 1.0e-7)
        self.assertDelta(ol.alpha(), 89.9997207, 1.0e-7)
        self.assertDelta(ol.beta(), 89.9997934, 1.0e-7)
        self.assertDelta(ol.gamma(), 89.9999396, 1.0e-7)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_peaks", ws_name + "_integrated"])


class SingleFilePredictPeaksUBFromFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "SingleFilePredictPeaksUBFromFindPeaksIntegrate"
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data")

        HB3AFindPeaks(InputWorkspace=ws_name + "_data", CellType="Orthorhombic", Centering="F", OutputWorkspace=ws_name + "_found_peaks")

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", UBWorkspace=ws_name + "_found_peaks", OutputWorkspace=ws_name + "_peaks")

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        peaks = mtd[ws_name + "_integrated"]

        self.assertEqual(peaks.getNumberPeaks(), 114)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.231258554, 1.0e-7)
        self.assertDelta(ol.b(), 5.257701834, 1.0e-7)
        self.assertDelta(ol.c(), 19.67041036, 1.0e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_found_peaks", ws_name + "_peaks", ws_name + "_integrated"])


class MultiFileFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "MultiFileFindPeaksIntegrate"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data"
        )

        HB3AFindPeaks(InputWorkspace=ws_name + "_data", CellType="Orthorhombic", Centering="F", OutputWorkspace=ws_name + "_peaks")

        peaks = mtd[ws_name + "_peaks"]
        self.assertEqual(peaks.getNumberOfEntries(), 2)

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        integrated = mtd[ws_name + "_integrated"]

        self.assertEqual(peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.261051697, 1.0e-7)
        self.assertDelta(ol.b(), 5.224167511, 1.0e-7)
        self.assertDelta(ol.c(), 19.689643636, 1.0e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), 10)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 6578.5329412, places=5)
        self.assertEqual(peak0.getBinCount(), 827)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 0)
        self.assertEqual(peak1.getK(), 4)
        self.assertEqual(peak1.getL(), 0)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 11316.7193962, places=5)
        self.assertEqual(peak1.getBinCount(), 134)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_peaks", ws_name + "_integrated"])


class MultiFilePredictPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "MultiFilePredictPeaksIntegrate"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data"
        )

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", OutputWorkspace=ws_name + "_peaks")

        peaks = mtd[ws_name + "_peaks"]

        self.assertEqual(peaks.getNumberOfEntries(), 2)

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        integrated = mtd[ws_name + "_integrated"]

        self.assertEqual(peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        # Should be the same as the UB from the data
        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.238389802, 1.0e-7)
        self.assertDelta(ol.b(), 5.238415883, 1.0e-7)
        self.assertDelta(ol.c(), 19.65194598, 1.0e-7)
        self.assertDelta(ol.alpha(), 89.9997207, 1.0e-7)
        self.assertDelta(ol.beta(), 89.9997934, 1.0e-7)
        self.assertDelta(ol.gamma(), 89.9999396, 1.0e-7)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), -11)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 127.13211, places=5)
        self.assertEqual(peak0.getBinCount(), 0)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 5)
        self.assertEqual(peak1.getK(), 0)
        self.assertEqual(peak1.getL(), 1)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 60.928141, places=5)
        self.assertEqual(peak1.getBinCount(), 0)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_peaks", ws_name + "_integrated"])


class MultiFilePredictPeaksUBFromFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = "MultiFilePredictPeaksUBFromFindPeaksIntegrate"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs", NormaliseBy="None", OutputWorkspace=ws_name + "_data"
        )

        HB3AFindPeaks(InputWorkspace=ws_name + "_data", CellType="Orthorhombic", Centering="F", OutputWorkspace=ws_name + "_found_peaks")

        found_peaks = mtd[ws_name + "_found_peaks"]
        self.assertEqual(found_peaks.getNumberOfEntries(), 2)
        self.assertEqual(found_peaks.getItem(0).getNumberPeaks(), 15)
        self.assertEqual(found_peaks.getItem(1).getNumberPeaks(), 15)

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", UBWorkspace=ws_name + "_found_peaks", OutputWorkspace=ws_name + "_peaks")

        peaks = mtd[ws_name + "_peaks"]
        self.assertEqual(peaks.getItem(0).getNumberPeaks(), 78)
        self.assertEqual(peaks.getItem(1).getNumberPeaks(), 113)

        HB3AIntegratePeaks(
            InputWorkspace=ws_name + "_data", PeaksWorkspace=ws_name + "_peaks", PeakRadius=0.25, OutputWorkspace=ws_name + "_integrated"
        )

        integrated = mtd[ws_name + "_integrated"]

        self.assertEqual(integrated.getNumberPeaks(), 191)
        self.assertEqual(peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        # should be the same as from MultiFileFindPeaksIntegrate test
        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.261051697, 1.0e-7)
        self.assertDelta(ol.b(), 5.224167511, 1.0e-7)
        self.assertDelta(ol.c(), 19.689643636, 1.0e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), -1)
        self.assertEqual(peak0.getK(), 1)
        self.assertEqual(peak0.getL(), 8)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 122.269802, places=5)
        self.assertEqual(peak0.getBinCount(), 0)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 3)
        self.assertEqual(peak1.getK(), 3)
        self.assertEqual(peak1.getL(), 3)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 101.175119, places=5)
        self.assertEqual(peak1.getBinCount(), 0)

        DeleteWorkspaces([ws_name + "_data", ws_name + "_found_peaks", ws_name + "_peaks", ws_name + "_integrated"])


class SatellitePeaksFakeData(systemtesting.MantidSystemTest):
    def runTest(self):
        # Na Mn Cl3
        # R -3 H (148)
        # 6.592 6.592 18.585177 90 90 120

        # UB/wavelength from /HFIR/HB3A/IPTS-25470/shared/autoreduce/HB3A_exp0769_scan0040.nxs

        ub = np.array(
            [[1.20297e-01, 1.70416e-01, 1.43000e-04], [8.16000e-04, -8.16000e-04, 5.38040e-02], [1.27324e-01, -4.05110e-02, -4.81000e-04]]
        )

        wavelength = 1.553

        # create fake MDEventWorkspace, similar to what is expected from exp769 after loading with HB3AAdjustSampleNorm
        MD_Q_sample = CreateMDWorkspace(
            Dimensions="3",
            Extents="-5,5,-5,5,-5,5",
            Names="Q_sample_x,Q_sample_y,Q_sample_z",
            Units="rlu,rlu,rlu",
            Frames="QSample,QSample,QSample",
        )

        inst = LoadEmptyInstrument(InstrumentName="HB3A")
        AddTimeSeriesLog(inst, "omega", "2010-01-01T00:00:00", 0.0)
        AddTimeSeriesLog(inst, "phi", "2010-01-01T00:00:00", 0.0)
        AddTimeSeriesLog(inst, "chi", "2010-01-01T00:00:00", 0.0)
        MD_Q_sample.addExperimentInfo(inst)
        SetUB(MD_Q_sample, UB=ub)

        ol = OrientedLattice()
        ol.setUB(ub)

        sg = SpaceGroupFactory.createSpaceGroup("R -3")

        hkl = []
        sat_hkl = []

        for h in range(0, 6):
            for k in range(0, 6):
                for L in range(0, 11):
                    if sg.isAllowedReflection([h, k, L]):
                        if h == k == L == 0:
                            continue
                        q = V3D(h, k, L)
                        q_sample = ol.qFromHKL(q)
                        if not np.any(np.array(q_sample) > 5):
                            hkl.append(q)
                            FakeMDEventData(MD_Q_sample, PeakParams="1000,{},{},{},0.05".format(*q_sample))
                        # satellite peaks at 0,0,+1.5
                        q = V3D(h, k, L + 1.5)
                        q_sample = ol.qFromHKL(q)
                        if not np.any(np.array(q_sample) > 5):
                            sat_hkl.append(q)
                            FakeMDEventData(MD_Q_sample, PeakParams="100,{},{},{},0.02".format(*q_sample))
                        # satellite peaks at 0,0,-1.5
                        q = V3D(h, k, L - 1.5)
                        q_sample = ol.qFromHKL(q)
                        if not np.any(np.array(q_sample) > 5):
                            sat_hkl.append(q)
                            FakeMDEventData(MD_Q_sample, PeakParams="100,{},{},{},0.02".format(*q_sample))

        # Check that this fake workpsace gives us the expected UB
        peaks = FindPeaksMD(MD_Q_sample, PeakDistanceThreshold=1, OutputType="LeanElasticPeak")
        FindUBUsingFFT(peaks, MinD=5, MaxD=20)
        ShowPossibleCells(peaks)
        SelectCellOfType(peaks, CellType="Rhombohedral", Centering="R", Apply=True)
        OptimizeLatticeForCellType(peaks, CellType="Hexagonal", Apply=True)
        found_ol = peaks.sample().getOrientedLattice()
        self.assertAlmostEqual(found_ol.a(), 6.592, places=2)
        self.assertAlmostEqual(found_ol.b(), 6.592, places=2)
        self.assertAlmostEqual(found_ol.c(), 18.585177, places=2)
        self.assertAlmostEqual(found_ol.alpha(), 90)
        self.assertAlmostEqual(found_ol.beta(), 90)
        self.assertAlmostEqual(found_ol.gamma(), 120)

        # nuclear peaks
        predict = HB3APredictPeaks(
            MD_Q_sample,
            Wavelength=wavelength,
            ReflectionCondition="Rhombohedrally centred, obverse",
            SatellitePeaks=True,
            IncludeIntegerHKL=True,
        )
        predict = HB3AIntegratePeaks(MD_Q_sample, predict, 0.25)

        self.assertEqual(predict.getNumberPeaks(), 66)
        # check that the found peaks are expected
        for n in range(predict.getNumberPeaks()):
            HKL = predict.getPeak(n).getHKL()
            self.assertTrue(HKL in hkl, msg=f"Peak {n} with HKL={HKL}")

        # magnetic peaks
        satellites = HB3APredictPeaks(
            MD_Q_sample,
            Wavelength=wavelength,
            ReflectionCondition="Rhombohedrally centred, obverse",
            SatellitePeaks=True,
            ModVector1="0,0,1.5",
            MaxOrder=1,
            IncludeIntegerHKL=False,
        )
        satellites = HB3AIntegratePeaks(MD_Q_sample, satellites, 0.1)

        self.assertEqual(satellites.getNumberPeaks(), 80)
        # check that the found peaks are expected
        for n in range(satellites.getNumberPeaks()):
            HKL = satellites.getPeak(n).getHKL()
            self.assertTrue(HKL in sat_hkl, msg=f"Peak {n} with HKL={HKL}")


class HB3AFindPeaksTest(systemtesting.MantidSystemTest):
    # test moved from unittests because of large memory usage
    def runTest(self):
        # Test with vanadium normalization to make sure UB matrix and peaks can still be found

        norm = HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0182.nxs", VanadiumFile="HB3A_exp0722_scan0220.nxs", NormaliseBy="None")
        peaks = HB3AFindPeaks(InputWorkspace=norm, CellType="Orthorhombic", Centering="F", PeakDistanceThreshold=0.25, Wavelength=1.008)
        # Verify UB and peaks were found
        self.assertTrue(HasUB(peaks))
        self.assertGreater(peaks.getNumberPeaks(), 0)
