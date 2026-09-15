# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the ISIS Reflectometry interface.

Replaces the scenarios of ``dev-docs/source/Testing/ReflectometryGUI/ReflectometryGUITests.rst``
that need neither the ISIS data archive nor the ICAT catalogue:

* scenario 2 - the interface opens with two batch tabs, they can be added and deleted, and deleting
  the last one immediately gives you another;
* scenario 3 - the instrument selector changes the default instrument for the whole of Mantid;
* scenario 7 - the Polarisation Corrections enable matrix, which is the richest purely-widget
  observation in the guide;
* scenario 12's availability rules - which Save tab options each file format allows.

The scenarios that reduce runs (4-6, 8, 11, 13) need INTER runs from the archive and, for 8, an ICAT
search; they are recorded in FUTURE_WORK.md.

Scenario 12's *file contents* checks - column counts, header markers, separators per format - are
the guide's most valuable and are also archive-gated, because there is nothing to save without a
reduction first. Only the option availability is checked here.
"""

import unittest

from reflectometry_gui_test_base import (
    BATCH_TAB_TITLES,
    POL_CORR_FILE_PATH,
    POL_CORR_NONE,
    POL_CORR_PARAMETER_FILE,
    POL_CORR_WORKSPACE,
    TAB_EXPERIMENT_SETTINGS,
    TAB_SAVE,
    ReflectometryGuiTestBase,
)
from qt_interaction_helpers import combo_items, process_events, select_combo

# the guide's scenario 3 instruments
INSTRUMENTS = ("INTER", "SURF", "CRISP", "POLREF", "OFFSPEC")


class ReflectometryGuiBatchTabsTest(ReflectometryGuiTestBase):
    """Guide scenario 2: adding and deleting batch tabs."""

    def test_opens_with_two_batches(self):
        with self.subTest("Scenario 2 / the interface opens with two batch tabs"):
            self.assertEqual(2, len(self.batch_titles()), f"the batches are {self.batch_titles()}")

        with self.subTest("Scenario 2 / each batch has the full set of tabs"):
            self.assertEqual(list(BATCH_TAB_TITLES), self.batch_tab_titles())

        with self.subTest("Scenario 2 / and so does the second batch"):
            self.assertEqual(list(BATCH_TAB_TITLES), self.batch_tab_titles(self.batch(1)))

    def test_deleting_every_batch_leaves_one(self):
        """'If all batch tabs are deleted ... a new one is automatically added.'"""
        tabs = self.main_tabs()

        # A bounded loop, not "while there are tabs left": the behaviour under test is that the
        # interface puts a new batch back as soon as the last one goes, so the count never reaches
        # zero and a condition waiting for it would spin for ever.
        for _attempt in range(tabs.count() + 1):
            tabs.tabCloseRequested.emit(tabs.count() - 1)
            process_events(3)

        with self.subTest("Scenario 2 / deleting the last batch immediately gives another"):
            self.assertEqual(1, tabs.count(), "the interface was left with no batch at all")

        with self.subTest("Scenario 2 / and the replacement is a working batch"):
            self.assertEqual(list(BATCH_TAB_TITLES), self.batch_tab_titles(tabs.widget(0)))


class ReflectometryGuiInstrumentTest(ReflectometryGuiTestBase):
    """Guide scenario 3: the instrument selector changes Mantid's default instrument."""

    def test_instrument_selector_changes_the_default_instrument(self):
        from mantid.kernel import config

        selector = self.batch_widget("instrumentSelector")

        with self.subTest("Scenario 3 / the interface offers the reflectometry instruments"):
            offered = combo_items(selector)
            for instrument in INSTRUMENTS:
                self.assertIn(instrument, offered)

        # restored by config_settings when the block ends, so the change cannot leak into the next
        # test - which matters because the guide's whole point is that it is process-wide
        with self.config_settings(default_instrument=config["default.instrument"]):
            for instrument in ("SURF", "POLREF", "INTER"):
                select_combo(selector, instrument)
                process_events(3)
                with self.subTest(f"Scenario 3 / selecting {instrument} changes it for the whole of Mantid"):
                    self.assertEqual(instrument, config["default.instrument"])


class ReflectometryGuiPolarisationCorrectionsTest(ReflectometryGuiTestBase):
    """Guide scenario 7: the Polarisation Corrections enable matrix.

    The guide walks the combo through its options and says which of the two dependent controls each
    one enables. That is the kind of wiring that quietly rots, and it needs no data at all.
    """

    def setUp(self):
        super(ReflectometryGuiPolarisationCorrectionsTest, self).setUp()
        self.show_batch_tab(TAB_EXPERIMENT_SETTINGS)
        self.pol_corr = self.combo("polCorrComboBox")
        self.instrument = self.batch_widget("instrumentSelector")

    def test_corrections_are_greyed_out_for_an_unpolarised_instrument(self):
        """Guide scenario 7: 'With INTER the Polarisation Corrections combo box should be greyed
        out.'"""
        with self.config_settings(default_instrument="INTER"):
            select_combo(self.instrument, "INTER")
            process_events(3)
            with self.subTest("Scenario 7 / INTER greys out the polarisation corrections"):
                self.assertFalse(self.pol_corr.isEnabled())

            select_combo(self.instrument, "OFFSPEC")
            process_events(3)
            with self.subTest("Scenario 7 / OFFSPEC enables them"):
                self.assertTrue(self.pol_corr.isEnabled())

    def test_polarisation_correction_options(self):
        # the options only apply on a polarised instrument; the guide switches to OFFSPEC before
        # this point, and on INTER the whole combo is greyed out
        with self.config_settings(default_instrument="OFFSPEC"):
            select_combo(self.instrument, "OFFSPEC")
            process_events(3)
            self._check_options()

    def _check_options(self):
        with self.subTest("Scenario 7 / the interface offers the four correction options"):
            self.assertEqual(
                [POL_CORR_NONE, POL_CORR_PARAMETER_FILE, POL_CORR_WORKSPACE, POL_CORR_FILE_PATH],
                combo_items(self.pol_corr),
            )

        # The guide names two dependent controls: "Fredrikze Input Spin State Order" and
        # "Polarization Efficiencies". The first is polCorrSpinStateEdit; the second is a workspace
        # selector added in code with no objectName, so the label beside it stands in for it - it is
        # enabled and disabled with the selector it labels.
        spin_state = self.batch_widget("polCorrSpinStateEdit")
        efficiencies_label = self.batch_widget("polCorrEfficienciesLabel")

        # The guide constrains the efficiencies input for all three options, but the spin state
        # order only for 'Workspace' - so only that is asserted for it. It happens to be enabled for
        # 'ParameterFile' too; the guide does not say it should not be, so nothing is claimed here.
        for option, enabled in ((POL_CORR_NONE, False), (POL_CORR_PARAMETER_FILE, False), (POL_CORR_WORKSPACE, True)):
            select_combo(self.pol_corr, option)
            process_events(3)
            with self.subTest(f"Scenario 7 / '{option}' {'enables' if enabled else 'leaves disabled'} the efficiencies input"):
                self.assertEqual(enabled, efficiencies_label.isEnabled())

        select_combo(self.pol_corr, POL_CORR_WORKSPACE)
        process_events(3)
        with self.subTest("Scenario 7 / 'Workspace' enables the Fredrikze input spin state order"):
            self.assertTrue(spin_state.isEnabled())

        select_combo(self.pol_corr, POL_CORR_FILE_PATH)
        process_events(3)

        with self.subTest("Scenario 7 / 'FilePath' also enables the efficiencies input"):
            self.assertTrue(efficiencies_label.isEnabled())


class ReflectometryGuiSaveOptionsTest(ReflectometryGuiTestBase):
    """Guide scenario 12: which Save tab options each file format allows.

    The guide states this per format - which of Header, separators, parameters, "Additional columns"
    and "Save multiple datasets to a single file" are available. Those rules are what stop a user
    asking for a combination the writer cannot produce, and none of them needs a reduction first.
    """

    def setUp(self):
        super(ReflectometryGuiSaveOptionsTest, self).setUp()
        self.show_batch_tab(TAB_SAVE)
        self.file_format = self.combo("fileFormatComboBox")

    def test_file_formats_offered(self):
        with self.subTest("Scenario 12 / the interface offers its save formats"):
            formats = combo_items(self.file_format)
            self.assertTrue(formats, "no file formats are offered")
            for expected in ("Custom format (*.dat)", "ORSO Ascii (*.ort)", "ORSO Nexus (*.orb)"):
                self.assertIn(expected, formats)

    def test_multiple_datasets_is_only_for_orso(self):
        """'Save multiple datasets to a single file should be disabled for all file formats apart
        from ORSO Ascii and ORSO Nexus.'

        Automatic saving is turned on first. The guide reaches this step from its Automatic Save
        section, where it has just ticked "Save automatically on completion", and the option is only
        live once that is on - without it every format would look disabled and the check would pass
        for the wrong reason.
        """
        from qt_interaction_helpers import set_checkbox

        # a save path first: the interface will not turn automatic saving on without somewhere to
        # save to, and the option under test is only live once it is on
        self.batch_widget("savePathEdit").setText(self.tmp_root)
        process_events(2)
        autosave = self.checkbox("saveReductionResultsCheckBox")
        set_checkbox(autosave, True)
        process_events(3)
        multiple = self.checkbox("multipleDatasetsCheckBox")

        for file_format in combo_items(self.file_format):
            select_combo(self.file_format, file_format)
            process_events(3)
            expected = "ORSO" in file_format
            with self.subTest(f"Scenario 12 / '{file_format}' {'allows' if expected else 'does not allow'} multiple datasets"):
                self.assertEqual(expected, multiple.isEnabled())

    def test_orso_greys_out_the_ascii_options(self):
        """For the ORSO formats the guide says the header, separator and parameter options are
        greyed out and 'Additional columns' becomes available instead."""
        header = self.checkbox("headerCheckBox")
        extra_columns = self.checkbox("extraColumnsCheckBox")

        select_combo(self.file_format, "Custom format (*.dat)")
        process_events(3)
        with self.subTest("Scenario 12 / the custom format offers the header option"):
            self.assertTrue(header.isEnabled())

        select_combo(self.file_format, "ORSO Ascii (*.ort)")
        process_events(3)
        with self.subTest("Scenario 12 / ORSO Ascii greys the header option out"):
            self.assertFalse(header.isEnabled())

        with self.subTest("Scenario 12 / and offers 'Additional columns' instead"):
            self.assertTrue(extra_columns.isEnabled())


if __name__ == "__main__":
    unittest.main()
