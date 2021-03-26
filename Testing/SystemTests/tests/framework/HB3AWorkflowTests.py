# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods
import systemtesting
from mantid.simpleapi import *


class SingleFileFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'SingleFileFindPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", OutputWorkspace=ws_name + '_data')

        HB3AFindPeaks(InputWorkspace=ws_name + '_data',
                      CellType="Orthorhombic",
                      Centering="F",
                      OutputWorkspace=ws_name + '_peaks')

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        peaks = mtd[ws_name + '_integrated']

        self.assertEqual(peaks.getNumberPeaks(), 15)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.231258554, 1.e-7)
        self.assertDelta(ol.b(), 5.257701834, 1.e-7)
        self.assertDelta(ol.c(), 19.67041036, 1.e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_peaks', ws_name + '_integrated'])


class SingleFilePredictPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'SingleFilePredictPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", OutputWorkspace=ws_name + '_data')

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", OutputWorkspace=ws_name + "_peaks")

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        peaks = mtd[ws_name + '_integrated']

        self.assertEqual(peaks.getNumberPeaks(), 112)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.238389802, 1.e-7)
        self.assertDelta(ol.b(), 5.238415883, 1.e-7)
        self.assertDelta(ol.c(), 19.65194598, 1.e-7)
        self.assertDelta(ol.alpha(), 89.9997207, 1.e-7)
        self.assertDelta(ol.beta(), 89.9997934, 1.e-7)
        self.assertDelta(ol.gamma(), 89.9999396, 1.e-7)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_peaks', ws_name + '_integrated'])


class SingleFilePredictPeaksUBFromFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'SingleFilePredictPeaksUBFromFindPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", OutputWorkspace=ws_name + '_data')

        HB3AFindPeaks(InputWorkspace=ws_name + '_data',
                      CellType="Orthorhombic",
                      Centering="F",
                      OutputWorkspace=ws_name + '_found_peaks')

        HB3APredictPeaks(InputWorkspace=ws_name + "_data",
                         UBWorkspace=ws_name + '_found_peaks',
                         OutputWorkspace=ws_name + '_peaks')

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        peaks = mtd[ws_name + '_integrated']

        self.assertEqual(peaks.getNumberPeaks(), 114)
        ol = peaks.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.231258554, 1.e-7)
        self.assertDelta(ol.b(), 5.257701834, 1.e-7)
        self.assertDelta(ol.c(), 19.67041036, 1.e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_found_peaks', ws_name + '_peaks', ws_name + '_integrated'])


class MultiFileFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'MultiFileFindPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs",
                             OutputWorkspace=ws_name + '_data')

        HB3AFindPeaks(InputWorkspace=ws_name + '_data',
                      CellType="Orthorhombic",
                      Centering="F",
                      OutputWorkspace=ws_name + '_peaks')

        peaks = mtd[ws_name + '_peaks']
        self.assertEqual(peaks.getNumberOfEntries(), 2)

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        integrated = mtd[ws_name + '_integrated']

        self.assertEqual(
            peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.261051697, 1.e-7)
        self.assertDelta(ol.b(), 5.224167511, 1.e-7)
        self.assertDelta(ol.c(), 19.689643636, 1.e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), 10)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 6581.5580392, places=5)
        self.assertEqual(peak0.getBinCount(), 827)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 0)
        self.assertEqual(peak1.getK(), 4)
        self.assertEqual(peak1.getL(), 0)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 11853.7538139, places=5)
        self.assertEqual(peak1.getBinCount(), 134)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_peaks', ws_name + '_integrated'])


class MultiFilePredictPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'MultiFilePredictPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs",
                             OutputWorkspace=ws_name + '_data')

        HB3APredictPeaks(InputWorkspace=ws_name + "_data", OutputWorkspace=ws_name + "_peaks")

        peaks = mtd[ws_name + "_peaks"]

        self.assertEqual(peaks.getNumberOfEntries(), 2)

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        integrated = mtd[ws_name + '_integrated']

        self.assertEqual(
            peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        # Should be the same as the UB from the data
        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.238389802, 1.e-7)
        self.assertDelta(ol.b(), 5.238415883, 1.e-7)
        self.assertDelta(ol.c(), 19.65194598, 1.e-7)
        self.assertDelta(ol.alpha(), 89.9997207, 1.e-7)
        self.assertDelta(ol.beta(), 89.9997934, 1.e-7)
        self.assertDelta(ol.gamma(), 89.9999396, 1.e-7)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), -11)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 127.20005, places=5)
        self.assertEqual(peak0.getBinCount(), 0)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 5)
        self.assertEqual(peak1.getK(), 0)
        self.assertEqual(peak1.getL(), 1)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 66.0945249, places=5)
        self.assertEqual(peak1.getBinCount(), 0)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_peaks', ws_name + '_integrated'])


class MultiFilePredictPeaksUBFromFindPeaksIntegrate(systemtesting.MantidSystemTest):
    def runTest(self):
        ws_name = 'MultiFilePredictPeaksUBFromFindPeaksIntegrate'
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs",
                             OutputWorkspace=ws_name + '_data')

        HB3AFindPeaks(InputWorkspace=ws_name + '_data',
                      CellType="Orthorhombic",
                      Centering="F",
                      OutputWorkspace=ws_name + '_found_peaks')

        found_peaks = mtd[ws_name + "_found_peaks"]
        self.assertEqual(found_peaks.getNumberOfEntries(), 2)
        self.assertEqual(found_peaks.getItem(0).getNumberPeaks(), 15)
        self.assertEqual(found_peaks.getItem(1).getNumberPeaks(), 15)

        HB3APredictPeaks(InputWorkspace=ws_name + "_data",
                         UBWorkspace=ws_name + '_found_peaks',
                         OutputWorkspace=ws_name + "_peaks")

        peaks = mtd[ws_name + "_peaks"]
        self.assertEqual(peaks.getItem(0).getNumberPeaks(), 78)
        self.assertEqual(peaks.getItem(1).getNumberPeaks(), 113)

        HB3AIntegratePeaks(InputWorkspace=ws_name + '_data',
                           PeaksWorkspace=ws_name + '_peaks',
                           PeakRadius=0.25,
                           OutputWorkspace=ws_name + '_integrated')

        integrated = mtd[ws_name + '_integrated']

        self.assertEqual(integrated.getNumberPeaks(), 191)
        self.assertEqual(
            peaks.getItem(0).getNumberPeaks() + peaks.getItem(1).getNumberPeaks(), integrated.getNumberPeaks())

        # should be the same as from MultiFileFindPeaksIntegrate test
        ol = integrated.sample().getOrientedLattice()
        self.assertDelta(ol.a(), 5.261051697, 1.e-7)
        self.assertDelta(ol.b(), 5.224167511, 1.e-7)
        self.assertDelta(ol.c(), 19.689643636, 1.e-7)
        self.assertEqual(ol.alpha(), 90)
        self.assertEqual(ol.beta(), 90)
        self.assertEqual(ol.gamma(), 90)

        peak0 = integrated.getPeak(0)
        self.assertEqual(peak0.getH(), -1)
        self.assertEqual(peak0.getK(), 1)
        self.assertEqual(peak0.getL(), 8)
        self.assertEqual(peak0.getRunNumber(), 182)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak0.getIntensity(), 124.408590, places=5)
        self.assertEqual(peak0.getBinCount(), 0)

        peak1 = integrated.getPeak(integrated.getNumberPeaks() - 1)
        self.assertEqual(peak1.getH(), 3)
        self.assertEqual(peak1.getK(), 3)
        self.assertEqual(peak1.getL(), 3)
        self.assertEqual(peak1.getRunNumber(), 183)
        self.assertAlmostEqual(peak1.getWavelength(), 1.008, places=5)
        self.assertAlmostEqual(peak1.getIntensity(), 101.3689957, places=5)
        self.assertEqual(peak1.getBinCount(), 0)

        DeleteWorkspaces([ws_name + '_data', ws_name + '_found_peaks', ws_name + '_peaks', ws_name + '_integrated'])
