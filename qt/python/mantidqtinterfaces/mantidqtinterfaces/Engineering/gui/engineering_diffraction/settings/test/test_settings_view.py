# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
import unittest

from mantid.api import FrameworkManager
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_view import SettingsView


@start_qapplication
class SettingsViewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # the C++ FileFinderWidgets in the dialog cannot be constructed before the framework is up
        FrameworkManager.Instance()

    def test_dialog_is_modal(self):
        # SettingsPresenter defers re-reading settings for a new RB number until show(), which is only
        # safe because the dialog is modal - nothing can change the RB while the dialog is up. If this
        # test fails because modality was deliberately dropped, SettingsPresenter.show() and
        # SettingsPresenter.set_rb_num() need revisiting rather than this assertion relaxing.
        view = SettingsView()

        self.assertTrue(view.isModal())

    @staticmethod
    def _grid_widths(view):
        # min and max are pinned together, so the boxes are exactly this wide once laid out
        return {(field.minimumWidth(), field.maximumWidth()) for field in view.direction_fields}

    def test_direction_boxes_are_wide_enough_for_a_three_decimal_component(self):
        view = SettingsView()

        widths = self._grid_widths(view)
        self.assertEqual(len(widths), 1)  # a grid: every box the same width
        self.assertGreater(widths.pop()[1], view.lineedit_RD0.fontMetrics().horizontalAdvance("-0.000"))

    def test_direction_boxes_grow_to_fit_the_longest_entry(self):
        view = SettingsView()
        before = view.lineedit_RD0.maximumWidth()

        # a direction adopted from a rotated reference frame is written to full precision
        view.set_rd_dir("0.8660254037844387,0.5,0.0")

        widths = self._grid_widths(view)
        # the whole grid follows the longest entry, so the columns stay aligned
        self.assertEqual(len(widths), 1)
        width = widths.pop()[1]
        self.assertGreater(width, before)
        self.assertGreaterEqual(width, view.lineedit_RD0.fontMetrics().horizontalAdvance("0.8660254037844387"))


if __name__ == "__main__":
    unittest.main()
