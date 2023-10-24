# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import Mock

import matplotlib

matplotlib.use("agg")

from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreatePeaksWorkspace, CreateMDWorkspace, SetUB, mtd, AddSampleLog
import numpy as np
from qtpy.QtWidgets import QApplication


@start_qapplication
class AddDeletePeaksTest(unittest.TestCase):
    """This will test going from a button_press_event in the sliceviewer
    matplotlib canvas all the way through to adding or deleting a peak
    in the attached peaksworkspace.

    """

    def tearDown(self):
        # Close windows after each test to ensure none windows remain before program destruction
        for widget in QApplication.topLevelWidgets():
            widget.close()
        QApplication.sendPostedEvents()
        QApplication.sendPostedEvents()

    def test_Q_Sample(self):
        md = CreateMDWorkspace(
            Dimensions=3, Extents="0,10,0,10,0,10", Names="x,y,z", Units="r.l.u.,r.l.u.,r.l.u.", Frames="QSample,QSample,QSample"
        )

        pw_name = "peaks_add_delete_test"
        CreatePeaksWorkspace(OutputType="LeanElasticPeak", NUmberOfPeaks=0, OutputWorkspace=pw_name)

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

        sliceViewer = SliceViewer(md)

        # select z=3.0 slice
        sliceViewer.view.dimensions.set_slicepoint((None, None, 3.0))

        event = Mock(inaxes=True, xdata=1.0, ydata=2.0)

        sliceViewer.canvas_clicked(event)

        # nothing should happen since peaksviewer isn't active
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

        # overlay_peaks_workspaces
        sliceViewer._create_peaks_presenter_if_necessary().overlay_peaksworkspaces([pw_name])

        sliceViewer.canvas_clicked(event)

        # nothing should happen since add/remove peak action not selected
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

        # click the "Add Peaks" button
        sliceViewer.view.peaks_view.peak_actions_view.ui.add_peaks_button.click()

        sliceViewer.canvas_clicked(event)  # should add a peak at (1, 2, 3)

        # peak should now be added
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 1)
        peak = mtd[pw_name].getPeak(0)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 2.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 3.0, delta=1e-10)
        self.assertEqual(peak.getH(), 0)
        self.assertEqual(peak.getK(), 0)
        self.assertEqual(peak.getL(), 0)

        # change to x-z slice, y=4, check that the transform is working
        sliceViewer.view.dimensions.dims[2].y_clicked()
        sliceViewer.view.dimensions.set_slicepoint((None, 4.0, None))

        sliceViewer.canvas_clicked(event)  # should add a peak at (1, 4, 2)

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 2)
        peak = mtd[pw_name].getPeak(1)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 4.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 2.0, delta=1e-10)
        self.assertEqual(peak.getH(), 0)
        self.assertEqual(peak.getK(), 0)
        self.assertEqual(peak.getL(), 0)

        # click the "Remove Peaks" button
        sliceViewer.view.peaks_view.peak_actions_view.ui.remove_peaks_button.click()

        sliceViewer.view.dimensions.set_slicepoint((None, 0.0, None))
        event = Mock(inaxes=True, xdata=1.0, ydata=1.0)
        sliceViewer.canvas_clicked(event)  # should remove the peak closest to (1, 0, 1)

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 1)

        # should be left with the seconds peak that was added
        peak = mtd[pw_name].getPeak(0)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 4.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 2.0, delta=1e-10)
        self.assertEqual(peak.getH(), 0)
        self.assertEqual(peak.getK(), 0)
        self.assertEqual(peak.getL(), 0)

        # remove another peaks, should be none remaining
        sliceViewer.canvas_clicked(event)  # should remove the peak closest to (1, 0, 1)
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

    def test_HKL(self):
        md = CreateMDWorkspace(Dimensions=3, Extents="0,10,0,10,0,10", Names="H,K,L", Units="r.l.u.,r.l.u.,r.l.u.", Frames="HKL,HKL,HKL")

        pw_name = "peaks_add_delete_test"
        CreatePeaksWorkspace(OutputType="LeanElasticPeak", NUmberOfPeaks=0, OutputWorkspace=pw_name)
        SetUB(pw_name, 2 * np.pi, 2 * np.pi, 4 * np.pi, u="0,0,1", v="1,0,0")

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

        sliceViewer = SliceViewer(md)

        # select z=3.0 slice
        sliceViewer.view.dimensions.set_slicepoint((None, None, 3.0))

        # overlay_peaks_workspaces
        sliceViewer._create_peaks_presenter_if_necessary().overlay_peaksworkspaces([pw_name])

        # click the "Add Peaks" button
        sliceViewer.view.peaks_view.peak_actions_view.ui.add_peaks_button.click()

        # click on 3 different points on the canvas
        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=1.0, ydata=2.0))  # should add a peak at HKL=(1, 2, 3)
        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=2.0, ydata=2.0))  # should add a peak at HKL=(2, 2, 3)
        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=1.5, ydata=1.5))  # should add a peak at HKL=(1.5, 1.5, 3)

        # peaks should be added
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 3)

        # (1, 2, 3)
        peak = mtd[pw_name].getPeak(0)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 2.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 1, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 3, delta=1e-10)

        # (2, 2, 3)
        peak = mtd[pw_name].getPeak(1)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 2.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 2.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 3, delta=1e-10)

        # (1.5, 1.5, 3)
        peak = mtd[pw_name].getPeak(2)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.5, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 1.5, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 3, delta=1e-10)

        # click the "Remove Peaks" button
        sliceViewer.view.peaks_view.peak_actions_view.ui.remove_peaks_button.click()

        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=2.0, ydata=1.9))  # should remove the peak closest to HKL=(2, 1.9, 3)

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 2)

        # should have deleted the (2, 2, 3) peak, leaving (1, 2, 3) and (1.5, 1.5, 3)

        # (1, 2, 3)
        peak = mtd[pw_name].getPeak(0)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 2.0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 1, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 3, delta=1e-10)

        # (1.5, 1.5, 3)
        peak = mtd[pw_name].getPeak(1)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 1.5, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 1.5, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 1.5, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 3, delta=1e-10)

    def test_HH0_00L(self):
        md = CreateMDWorkspace(
            Dimensions=3, Extents="-5,5,-5,5,-5,5", Names="[H,H,0],[0,0,L],[H,-H,0]", Units="r.l.u.,r.l.u.,r.l.u.", Frames="HKL,HKL,HKL"
        )

        AddSampleLog(md, LogName="sample")
        md.getExperimentInfo(0).run().addProperty("W_MATRIX", [1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0], True)

        pw_name = "peaks_add_delete_test"
        SetUB(md, 5, 5, 5)
        CreatePeaksWorkspace(InstrumentWorkspace=md, OutputType="LeanElasticPeak", NUmberOfPeaks=0, OutputWorkspace=pw_name)

        self.assertEqual(mtd[pw_name].getNumberPeaks(), 0)

        sliceViewer = SliceViewer(md)

        # overlay_peaks_workspaces
        sliceViewer._create_peaks_presenter_if_necessary().overlay_peaksworkspaces([pw_name])

        # click the "Add Peaks" button
        sliceViewer.view.peaks_view.peak_actions_view.ui.add_peaks_button.click()

        # click on 2 different points on the canvas
        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=2.0, ydata=0.0))  # should add a peak at HKL=(2, 2, 0)
        sliceViewer.canvas_clicked(Mock(inaxes=True, xdata=0.0, ydata=2.0))  # should add a peak at HKL=(0, 0, 2)

        # peaks should be added
        self.assertEqual(mtd[pw_name].getNumberPeaks(), 2)

        # (2, 2, 0)
        peak = mtd[pw_name].getPeak(0)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], np.pi * 4 / 5, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], 0, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], np.pi * 4 / 5, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 2, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 0, delta=1e-10)

        # (0, 0, 2)
        peak = mtd[pw_name].getPeak(1)
        q_sample = peak.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], 0, delta=1e-10)
        self.assertAlmostEqual(q_sample[1], np.pi * 4 / 5, delta=1e-10)
        self.assertAlmostEqual(q_sample[2], 0, delta=1e-10)
        self.assertAlmostEqual(peak.getH(), 0, delta=1e-10)
        self.assertAlmostEqual(peak.getK(), 0, delta=1e-10)
        self.assertAlmostEqual(peak.getL(), 2, delta=1e-10)


if __name__ == "__main__":
    unittest.main()
