# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the ALFView interface.

Replaces what can be driven of ``dev-docs/source/Testing/Direct/ALFViewTests.rst``.

Every scenario in that guide loads ALF run 82301 from the ISIS archive, so the loading test declares
it and skips without it. What runs everywhere is the structure the guide walks through first: the
two tabs, and the normalisation control that scenario 1 uses to divide the sample by the vanadium.

Three parts of the guide are not covered and are recorded in FUTURE_WORK.md:

* the tube selection of scenario 3 - clicking and rubber-band selecting tubes on the instrument
  view, which is a C++ widget driven by mouse events on a rendered scene;
* the colour observations throughout ("a flat color", "a darker color", "a red line labelled Fitted
  Data"), which need a rendered image to judge;
* scenario 6, which closes and reopens Mantid itself.
"""

import unittest

from alf_view_gui_test_base import ALF_RUN, TAB_PICK, TAB_RENDER, AlfViewGuiTestBase
from qt_interaction_helpers import combo_items, process_events


class AlfViewGuiLayoutTest(AlfViewGuiTestBase):
    """The interface opens with the two tabs the guide works through."""

    def test_tabs(self):
        with self.subTest("ALFView / the interface offers its Render and Pick tabs"):
            self.assertEqual([TAB_RENDER, TAB_PICK], self.tab_titles())

        for title in (TAB_RENDER, TAB_PICK):
            with self.subTest(f"ALFView / the '{title}' tab can be selected"):
                self.assertTrue(self.show_tab(title).isVisible())

    def test_normalisation_options(self):
        """Scenario 1 divides the sample by the vanadium, which is what this control selects."""
        normalisation = self.widget("normTypeOpt")

        with self.subTest("ALFView / the interface offers its normalisation options"):
            self.assertTrue(combo_items(normalisation), "the normalisation combo is empty")

        with self.subTest("ALFView / and one of them is selected to begin with"):
            self.assertTrue(normalisation.currentText())


class AlfViewGuiLoadingTest(AlfViewGuiTestBase):
    """Guide scenario 1: loading a sample run. Needs the ISIS data archive."""

    def required_files(self):
        return (ALF_RUN,)

    def test_loading_a_sample_run(self):
        from mantid.api import AnalysisDataService as ADS
        from qtpy.QtWidgets import QLabel, QLineEdit
        from qt_interaction_helpers import child_named, set_line_edit, wait_until

        self.show_tab(TAB_RENDER)
        finder = self.widget("browseBtn").parent()
        editor = child_named(finder, "fileEditor", QLineEdit)
        set_line_edit(editor, ALF_RUN, expect_valid=False)
        valid_label = finder.findChild(QLabel, "valid")
        if valid_label is not None:
            wait_until(lambda: not valid_label.isVisible(), timeout=60.0, msg=f"the file finder to resolve {ALF_RUN}")
        process_events(5)

        with self.subTest("Scenario 1 / the sample run is loaded"):
            self.assertTrue(ADS.getObjectNames(), f"nothing was loaded: {self.message_box_messages}")


if __name__ == "__main__":
    unittest.main()
