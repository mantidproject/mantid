# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Inelastic Data Processor interface.

Covers what can be driven of ``dev-docs/source/Testing/Inelastic/DataProcessorTests.rst``: loading a
run into the Symmetrise tab, which is the step every one of that guide's five scenarios starts with.

**Why the reductions themselves are not here.** Each of the guide's scenarios sets its energy or Q
range before pressing Run, and on these tabs that range lives in a Qt property browser driven by a
pair of draggable markers on the preview plot - not in a named widget. Pressing Run without it does
not reduce: the interface refuses and says so in a modal message box (harmless now that
``dismiss_modal_dialogs`` is in the base setUp, but still no reduction). Driving the property browser
is the missing piece, and it is the same piece the Corrections and QENS Fitting guides need. It is
recorded in FUTURE_WORK.md.

Loading is worth having on its own: it is the step the guide repeats five times, it exercises the
file finder and the input-type switch, and it is where a broken data search path shows up first.
"""

import unittest

from inelastic_gui_test_base import (
    INTERFACE_DATA_PROCESSOR,
    IRIS_REDUCED,
    TAB_MOMENTS,
    TAB_SYMMETRISE,
    TABS_DATA_PROCESSOR,
    InelasticGuiTestBase,
)
from qt_interaction_helpers import combo_items, process_events

# the workspace name the loaded file takes in the ADS
LOADED_WORKSPACE = "irs26176_graphite002_red"


class InelasticGuiDataProcessorLoadingTest(InelasticGuiTestBase):
    """Loading a reduced run into the Data Processor tabs."""

    INTERFACE = INTERFACE_DATA_PROCESSOR
    TABS = TABS_DATA_PROCESSOR

    def required_files(self):
        return (IRIS_REDUCED,)

    def test_loading_into_symmetrise(self):
        from mantid.api import AnalysisDataService as ADS

        page = self.show_tab(TAB_SYMMETRISE)

        with self.subTest("Scenario 1 / the tab offers both File and Workspace as input"):
            self.assertEqual(["File", "Workspace"], combo_items(self.combo("cbInputType", page)))

        self.load_file(page, IRIS_REDUCED)

        with self.subTest("Scenario 1 / the interface is ready to run once a run is chosen"):
            self.assertTrue(self.button("pbRun", page).isEnabled())

        # the tab loads its input when it is run, not when the file is chosen
        created = self.press_run(page)

        with self.subTest("Scenario 1 / running the tab loads the chosen run"):
            self.assertIn(LOADED_WORKSPACE, created, f"the ADS holds {created}")
            self.assertGreater(ADS.retrieve(LOADED_WORKSPACE).getNumberHistograms(), 0)

    def test_loading_into_moments(self):
        """Guide scenario 4 loads an ``_sqw`` workspace into Moments; the loading path is the same
        one, and this checks the tab accepts a file at all."""
        from mantid.api import AnalysisDataService as ADS

        from qt_interaction_helpers import set_checkbox

        page = self.show_tab(TAB_MOMENTS)
        self.load_file(page, IRIS_REDUCED)

        with self.subTest("Scenario 4 / the Moments tab accepts a file"):
            self.assertTrue(self.button("pbRun", page).isEnabled())

        with self.subTest("Scenario 4 / the scale option can be turned on"):
            scale = self.widget("ckScale", parent=page)
            set_checkbox(scale, True)
            process_events(2)
            self.assertTrue(self.widget("spScale", parent=page).isEnabled())

        created = self.press_run(page)

        with self.subTest("Scenario 4 / running the tab loads the chosen run"):
            self.assertIn(LOADED_WORKSPACE, created, f"the ADS holds {created}")
            self.assertTrue(ADS.doesExist(LOADED_WORKSPACE))


if __name__ == "__main__":
    unittest.main()
