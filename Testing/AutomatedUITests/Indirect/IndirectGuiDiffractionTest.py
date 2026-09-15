# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Indirect Diffraction interface.

Replaces ``dev-docs/source/Testing/Indirect/DiffractionTests.rst``.

Two classes:

* ``IndirectGuiDiffractionGroupingTest`` - which grouping options the interface offers and which
  extra input each one reveals.
* ``IndirectGuiDiffractionReductionTest`` - the reductions, whose observations are the guide's
  spectrum counts.

Both declare the runs they need (IRIS 26176, OSIRIS 89813/89757) and skip cleanly without them.

The guide's three "should not be allowed" cases are deliberately not covered - see the comment in
``IndirectGuiDiffractionReductionTest``, and FUTURE_WORK.md.
"""

import unittest

from indirect_gui_test_base import (
    GROUPING_ALL,
    GROUPING_CUSTOM,
    GROUPING_FILE,
    GROUPING_GROUPS,
    INTERFACE_DIFFRACTION,
    IRIS_DIFFRACTION_RUN,
    OSIRIS_CAL_FILE,
    OSIRIS_DIFFRACTION_RUN,
    OSIRIS_VANADIUM_RUN,
    IndirectGuiTestBase,
)
from qtpy.QtWidgets import QCheckBox

from qt_interaction_helpers import click, combo_items, process_events, select_combo, set_checkbox, set_line_edit, wait_until

# A diffraction reduction loads a raw run and focuses it, so it is slower than anything else in this
# suite; the wait is bounded well above what it takes rather than left to the ten second default.
REDUCTION_TIMEOUT = 180.0

# How long to wait for Run to go *down*, which is the sign a reduction actually started. A refused
# one never disables the button, so this is also the whole cost of proving a negative case - which
# is why it is short: a reduction of these runs starts immediately.
STARTED_TIMEOUT = 10.0

# IRIS's diffraction spectra, which is what the guide's "spectra numbers outside 105-112" is about
IRIS_SPECTRA_MIN, IRIS_SPECTRA_MAX = 105, 112

# the guide's own custom grouping strings
VALID_CUSTOM_GROUPING = "105-108,109-112"
OUT_OF_RANGE_CUSTOM_GROUPING = "1-50,51-100"

# "Groups with more than 8 groups should not be allowed" - IRIS has 8 diffraction spectra
TOO_MANY_GROUPS = 9


class IndirectGuiDiffractionGroupingTest(IndirectGuiTestBase):
    """Guide scenario 1's grouping options: which the interface offers, and which input each reveals.

    A sample run is set even though nothing here reduces one, because several of the interface's
    controls are only populated once it knows what it is reducing.
    """

    INTERFACE = INTERFACE_DIFFRACTION

    def required_files(self):
        return (IRIS_DIFFRACTION_RUN,)

    def setUp(self):
        super(IndirectGuiDiffractionGroupingTest, self).setUp()
        self.set_instrument("IRIS")
        self.set_finder("rfSampleFiles", IRIS_DIFFRACTION_RUN)
        process_events(3)

    def test_grouping_options(self):
        grouping = self.combo("cbGroupingOptions")

        with self.subTest("Scenario 1 / the interface offers All, File, Groups and Custom"):
            self.assertEqual([GROUPING_ALL, GROUPING_FILE, GROUPING_GROUPS, GROUPING_CUSTOM], combo_items(grouping))

        with self.subTest("Scenario 1 / the default detector grouping is All"):
            self.assertEqual(GROUPING_ALL, grouping.currentText())

        self._check_conditional_inputs()

    def _check_conditional_inputs(self):
        """Each grouping option reveals only the input it needs."""
        grouping = self.combo("cbGroupingOptions")
        groups_spin = self.spin_box("spNumberGroups")
        custom_edit = self.line_edit("leCustomGroups")

        select_combo(grouping, GROUPING_ALL)
        process_events(2)
        with self.subTest("Scenario 1 / 'All' needs no extra input"):
            self.assertFalse(groups_spin.isVisible())
            self.assertFalse(custom_edit.isVisible())

        select_combo(grouping, GROUPING_GROUPS)
        process_events(2)
        with self.subTest("Scenario 1 / 'Groups' reveals the number of groups only"):
            self.assertTrue(groups_spin.isVisible())
            self.assertFalse(custom_edit.isVisible())

        select_combo(grouping, GROUPING_CUSTOM)
        process_events(2)
        with self.subTest("Scenario 1 / 'Custom' reveals the grouping string only"):
            self.assertTrue(custom_edit.isVisible())
            self.assertFalse(groups_spin.isVisible())


class IndirectGuiDiffractionReductionTest(IndirectGuiTestBase):
    """Guide scenarios 1 and 2, the reductions themselves. Needs the ISIS data archive."""

    INTERFACE = INTERFACE_DIFFRACTION

    def required_files(self):
        return (IRIS_DIFFRACTION_RUN, OSIRIS_DIFFRACTION_RUN, OSIRIS_VANADIUM_RUN, OSIRIS_CAL_FILE)

    def test_iris_detector_grouping(self):
        """Scenario 1: All gives 1 spectrum, Groups=4 gives 4, Custom gives 2."""
        self.set_instrument("IRIS")
        self.set_run(IRIS_DIFFRACTION_RUN)

        cases = ((GROUPING_ALL, None, 1), (GROUPING_GROUPS, 4, 4), (GROUPING_CUSTOM, VALID_CUSTOM_GROUPING, 2))
        for grouping, setting, expected in cases:
            self._select_grouping(grouping, setting)
            output = self.run_reduction()
            with self.subTest(f"Scenario 1 / '{grouping}' produces {expected} spectra"):
                self.assertEqual(expected, self.spectrum_count(output))

    # Guide scenario 1's three negative cases - a custom grouping outside the instrument's spectra,
    # more groups than it has, and an empty grouping file - are NOT covered, and must not be added
    # back without first solving the problem below.
    #
    # The interface does not refuse them up front: Run stays enabled, the reduction starts, and the
    # refusal arrives as a modal message box raised from C++. Nothing in Python can intercept that -
    # patch_error_messages and patch_confirmation_box both work by replacing a Python-side symbol -
    # so the box sits there with no one to dismiss it and the run hangs until CTest kills it. That
    # is exactly the failure mode the harness's hang dump exists to make visible, and it was
    # observed here before this test was removed. See FUTURE_WORK.md.

    def test_osiris_diffraction(self):
        """Scenario 2: three output workspaces, each with the grouping's number of spectra."""
        from mantid.api import AnalysisDataService as ADS

        self.set_instrument("OSIRIS")
        # the guide writes it "Diffonly"; the combo spells it lower case
        self.set_reflection("diffonly")
        self.set_run(OSIRIS_DIFFRACTION_RUN)
        self.set_vanadium(OSIRIS_VANADIUM_RUN)
        self.set_calibration_file(OSIRIS_CAL_FILE)

        cases = ((GROUPING_ALL, None, 1), (GROUPING_GROUPS, 5, 5), (GROUPING_CUSTOM, "3-500,501-962", 2))
        for grouping, setting, expected in cases:
            self._select_grouping(grouping, setting)
            self.run_reduction()

            with self.subTest(f"Scenario 2 / '{grouping}' produces the three output workspaces"):
                produced = {
                    suffix: [name for name in ADS.getObjectNames() if name.endswith(suffix)] for suffix in ("_dRange", "_q", "_tof")
                }
                for suffix, names in produced.items():
                    self.assertTrue(names, f"no workspace ending {suffix} was created")

            for suffix in ("_dRange", "_q", "_tof"):
                names = [name for name in ADS.getObjectNames() if name.endswith(suffix)]
                with self.subTest(f"Scenario 2 / '{grouping}' gives {expected} spectra in the {suffix} workspace"):
                    self.assertEqual(expected, self.spectrum_count(names[-1]))

    # ------------------------------------------------------------------ driving the reduction

    def _select_grouping(self, grouping, setting):
        select_combo(self.combo("cbGroupingOptions"), grouping)
        process_events(2)
        if grouping == GROUPING_GROUPS:
            self.spin_box("spNumberGroups").setValue(setting)
        elif grouping == GROUPING_CUSTOM:
            set_line_edit(self.line_edit("leCustomGroups"), setting, expect_valid=False)
        process_events(3)

    def set_run(self, run):
        self.set_finder("rfSampleFiles", run)

    def set_vanadium(self, run):
        set_checkbox(self.window.findChild(QCheckBox, "ckUseVanadium"), True)
        process_events(2)
        self.set_finder("rfVanFile", run)

    def set_calibration_file(self, filename):
        set_checkbox(self.window.findChild(QCheckBox, "ckUseCalib"), True)
        process_events(2)
        self.set_finder("rfCalFile", filename)

    def attempt_reduction(self, timeout=REDUCTION_TIMEOUT):
        """Press Run and wait for whatever comes back, without insisting anything does.

        The ADS is cleared first so that "what did this reduction produce" is the whole of it: the
        interface reuses one output name per run and grouping, so comparing before and after would
        see no change the second time the same run is reduced.

        These interfaces run their reduction on a background thread but expose no ``AsyncTask``, so
        there is nothing for ``wait_for_async_task`` to watch. What is observable is the Run button,
        which the interface disables for the duration; the wait ends when it comes back and the
        event queue has drained.
        """
        from mantid.api import AnalysisDataService as ADS

        ADS.clear()
        run_button = self.button("pbRun")
        click(run_button)

        # Two phases, and the first is allowed to time out. A reduction that actually starts
        # disables Run for its duration; one the interface refuses never disables it at all, so
        # waiting for it to go down is how the two are told apart - and waiting for it to come back
        # up is how the first kind is waited on. Watching the ADS instead would not work: the output
        # name is the same every time, so "the workspaces changed" is not observable on a re-run.
        try:
            wait_until(lambda: not run_button.isEnabled(), timeout=STARTED_TIMEOUT, msg="the reduction to start")
        except RuntimeError:
            process_events(3)
            return sorted(ADS.getObjectNames())

        wait_until(run_button.isEnabled, timeout=timeout, msg="the reduction to finish")
        process_events(3)
        return sorted(ADS.getObjectNames())

    def run_reduction(self):
        """Press Run and require that it produced something."""
        created = self.attempt_reduction()
        self.assertTrue(created, "the reduction produced no workspaces")
        return created[-1]

    @staticmethod
    def spectrum_count(name):
        from mantid.api import AnalysisDataService as ADS

        return ADS.retrieve(name).getNumberHistograms()


if __name__ == "__main__":
    unittest.main()
