# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import create_autospec

# 3rdparty imports
from mantidqt.widgets.sliceviewer.peaksviewer.actions import PeakActionsView
from mantidqt.widgets.sliceviewer.peaksviewer.presenter import PeaksViewerCollectionPresenter
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class PeaksActionViewTest(unittest.TestCase):
    def test_add_peak_mode(self):
        view = PeakActionsView()

        self.assertFalse(view.erasing_mode_on)
        self.assertFalse(view.adding_mode_on)

        mock_PeaksViewerCollectionPresenter = create_autospec(PeaksViewerCollectionPresenter, instance=True)

        view.subscribe(mock_PeaksViewerCollectionPresenter)

        mock_PeaksViewerCollectionPresenter.deactivate_zoom_pan.assert_not_called()

        # click the "Add Peaks" button to turn on
        view.ui.add_peaks_button.click()

        self.assertFalse(view.erasing_mode_on)
        self.assertTrue(view.adding_mode_on)

        mock_PeaksViewerCollectionPresenter.deactivate_zoom_pan.assert_called_once_with(True)

        mock_PeaksViewerCollectionPresenter.deactivate_zoom_pan.reset_mock()

        # click the "Add Peaks" button to turn off
        view.ui.add_peaks_button.click()

        self.assertFalse(view.erasing_mode_on)
        self.assertFalse(view.adding_mode_on)

        mock_PeaksViewerCollectionPresenter.deactivate_zoom_pan.assert_called_once_with(False)

    def test_set_peaksworkspace(self):
        view = PeakActionsView()

        self.assertEqual(view.active_peaksworkspace, "")

        view.set_peaksworkspace(["ws1", "ws2", "ws3"])

        self.assertEqual(view.active_peaksworkspace, "ws1")

        # select ws3 from the combobox
        view.ui.active_peaks_combobox.setCurrentText("ws3")
        self.assertEqual(view.active_peaksworkspace, "ws3")

        # remove ws1, check that ws3 is still active
        view.set_peaksworkspace(["ws2", "ws3"])
        self.assertEqual(view.active_peaksworkspace, "ws3")

        # remove ws3, check that ws2 is now active
        view.set_peaksworkspace(["ws2"])
        self.assertEqual(view.active_peaksworkspace, "ws2")

        # add them all back, check that ws2 is still active
        view.set_peaksworkspace(["ws1", "ws2", "ws3"])
        self.assertEqual(view.active_peaksworkspace, "ws2")


if __name__ == "__main__":
    unittest.main()
