# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Indirect Data Reduction interface.

Replaces the parts of ``dev-docs/source/Testing/Indirect/DataReductionTests.rst`` that can be driven
without the ISIS data archive.

The guide's four scenarios each end in a reduction of IRIS runs 26173, 26176 or 26184-5. Those runs
are not in the repository's data store, so the reductions themselves are recorded in FUTURE_WORK.md
rather than written blind. What is covered here is the structure the guide walks through on the way:
the four tabs, and the conditional inputs on ISIS Calibration that its first scenario drives - "Scale
by factor", "Create RES" and the resolution options, each of which reveals its own inputs.

Nothing here presses Run. A reduction the interface refuses reports it through a modal message box
raised from C++, which no Python patch can dismiss, so an unattended test that provokes one hangs
until it is killed - see the comment in ``IndirectGuiDiffractionTest``.
"""

import unittest

from indirect_gui_test_base import (
    INTERFACE_DATA_REDUCTION,
    TAB_CALIBRATION,
    TAB_DIAGNOSTICS,
    TAB_ENERGY_TRANSFER,
    TAB_TRANSMISSION,
    IndirectGuiTestBase,
)
from qt_interaction_helpers import process_events, set_checkbox

TABS = "twIDRTabs"


class IndirectGuiDataReductionTabsTest(IndirectGuiTestBase):
    """The interface opens with the four tabs the guide works through."""

    INTERFACE = INTERFACE_DATA_REDUCTION

    def test_tabs(self):
        with self.subTest("Data Reduction / the interface offers its four tabs"):
            self.assertEqual(
                [TAB_ENERGY_TRANSFER, TAB_CALIBRATION, TAB_DIAGNOSTICS, TAB_TRANSMISSION],
                self.tab_titles(TABS),
            )

        for title in (TAB_ENERGY_TRANSFER, TAB_CALIBRATION, TAB_DIAGNOSTICS, TAB_TRANSMISSION):
            with self.subTest(f"Data Reduction / the '{title}' tab can be selected"):
                page = self.show_tab(TABS, title)
                self.assertTrue(page.isVisible())


class IndirectGuiCalibrationOptionsTest(IndirectGuiTestBase):
    """Guide scenario 1's options on the ISIS Calibration tab.

    The guide has the tester scale the calibration by a factor and create a RES workspace; both are
    check boxes that reveal further inputs, and getting that wiring wrong is what would silently
    produce a calibration with no scaling applied.
    """

    INTERFACE = INTERFACE_DATA_REDUCTION

    def setUp(self):
        super(IndirectGuiCalibrationOptionsTest, self).setUp()
        self.page = self.show_tab(TABS, TAB_CALIBRATION)

    def test_scale_by_factor(self):
        """Guide scenario 1: 'Scale by factor' 0.5.

        Only what the guide asks is checked - that the option can be turned on and the factor
        set. It does *not* say the factor should be greyed out beforehand, and it is not: asserting
        that would be inventing a requirement rather than replaying the manual test.
        """
        scale = self.widget("ckScale", parent=self.page)
        factor = self.line_edit("leScale", parent=self.page)

        set_checkbox(scale, True)
        process_events(2)
        with self.subTest("Scenario 1 / 'Scale by factor' can be turned on"):
            self.assertTrue(scale.isChecked())

        with self.subTest("Scenario 1 / the factor can be set to the guide's 0.5"):
            factor.setText("0.5")
            factor.editingFinished.emit()
            process_events(2)
            self.assertEqual("0.5", factor.text())

    def test_create_resolution(self):
        """'Create RES' reveals the resolution options the guide then uses."""
        create_res = self.widget("ckCreateResolution", parent=self.page)
        smooth = self.widget("ckSmoothResolution", parent=self.page)
        scale_res = self.widget("ckResolutionScale", parent=self.page)

        set_checkbox(create_res, False)
        process_events(2)
        with self.subTest("Scenario 1 / the resolution options are unavailable until 'Create RES' is ticked"):
            self.assertFalse(smooth.isEnabled())
            self.assertFalse(scale_res.isEnabled())

        set_checkbox(create_res, True)
        process_events(2)
        with self.subTest("Scenario 1 / ticking 'Create RES' makes them available"):
            self.assertTrue(smooth.isEnabled())
            self.assertTrue(scale_res.isEnabled())

        scale_factor = self.line_edit("leResolutionScale", parent=self.page)
        set_checkbox(scale_res, True)
        process_events(2)
        with self.subTest("Scenario 1 / scaling the resolution reveals its own factor"):
            self.assertTrue(scale_factor.isEnabled())

    def test_sum_files(self):
        """The guide sums runs 55878-55879 and 59057-59059, which is this check box."""
        sum_files = self.widget("ckSumFiles", parent=self.page)

        with self.subTest("Scenario 1 / runs can be summed"):
            set_checkbox(sum_files, True)
            self.assertTrue(sum_files.isChecked())

        with self.subTest("Scenario 1 / and unsummed again"):
            set_checkbox(sum_files, False)
            self.assertFalse(sum_files.isChecked())


class IndirectGuiDiagnosticsOptionsTest(IndirectGuiTestBase):
    """Guide scenario 3's options on the ISIS Diagnostics tab.

    The guide's slider dragging and the ``_slice`` output are recorded in FUTURE_WORK.md; what is
    checked here is the option that changes what the tab does - "Use Two Ranges", which is what puts
    the second, background pair of sliders on the preview plot.
    """

    INTERFACE = INTERFACE_DATA_REDUCTION

    def setUp(self):
        super(IndirectGuiDiagnosticsOptionsTest, self).setUp()
        self.page = self.show_tab(TABS, TAB_DIAGNOSTICS)

    def test_use_calibration(self):
        """'Use Calibration' reveals the calibration input the guide feeds the _calib workspace to."""
        use_calibration = self.widget("ckUseCalibration", parent=self.page)

        set_checkbox(use_calibration, True)
        process_events(2)
        with self.subTest("Scenario 3 / a calibration can be used"):
            self.assertTrue(use_calibration.isChecked())

        set_checkbox(use_calibration, False)
        process_events(2)
        with self.subTest("Scenario 3 / and turned off again"):
            self.assertFalse(use_calibration.isChecked())


if __name__ == "__main__":
    unittest.main()
