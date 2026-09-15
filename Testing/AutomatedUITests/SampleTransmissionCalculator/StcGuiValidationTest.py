# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Sample Transmission Calculator's input validation.

Covers the "Validation of Single Wavelength Range", "Multiple Wavelength Range" and "Chemical
Formula Validation" sections of
``dev-docs/source/Testing/General/SampleTransmissionCalculatorTestGuide.rst``.

The guide describes two different kinds of rejection and they are checked differently here:

* a range the *model* refuses - "there should be a warning message in red at the bottom of the
  window". The colour is not asserted (it is set once in the view's constructor and never varies);
  what is checked is that the warning text arrived and that the offending input was flagged.
* a value the *spin box* refuses - "it should not allow you to input ... (no warning will be
  printed)". This has to be checked by typing, because assigning through ``setValue`` bypasses the
  validator that does the refusing, so a test written that way would pass whatever the widget did.
"""

import unittest

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from sample_transmission_gui_test_base import GUIDE_HIGH, GUIDE_LOW, GUIDE_WIDTH, SampleTransmissionGuiTestBase
from qt_interaction_helpers import process_events, set_line_edit

# a sample that is itself valid, so the only thing under test in this module is the range or the
# formula the step is varying
VALID_FORMULA = "V"
VALID_DENSITY = 6.0
VALID_THICKNESS = 0.5


def _type_into(spin_box, text):
    """Type into a spin box's line edit, keystroke by keystroke.

    The validator only sees keystrokes. ``setValue`` silently clamps out-of-range values and never
    consults it at all, so it cannot be used to ask "would the widget let a user enter this?".
    """
    line_edit = spin_box.lineEdit()
    line_edit.setFocus()
    line_edit.selectAll()
    QTest.keyClick(line_edit, Qt.Key_Backspace)
    QTest.keyClicks(line_edit, text)
    process_events()
    return line_edit.text()


class StcGuiSingleRangeValidationTest(SampleTransmissionGuiTestBase):
    """Guide section 'Validation of Single Wavelength Range'."""

    def setUp(self):
        super(StcGuiSingleRangeValidationTest, self).setUp()
        self.set_sample(VALID_FORMULA, VALID_DENSITY, VALID_THICKNESS)

    def test_single_wavelength_range_validation(self):
        self._check_rejected_ranges()
        self._check_spin_boxes_refuse_bad_input()
        self._check_emptied_box_is_restored()

    def _check_rejected_ranges(self):
        cases = {
            "step 1 (Low > High)": (7.8, GUIDE_WIDTH, 1.8),
            "step 2 (Width > High - Low)": (GUIDE_LOW, 10.0, GUIDE_HIGH),
            "step 3 (Width = 0)": (GUIDE_LOW, 0.0, GUIDE_HIGH),
        }
        for label, (low, width, high) in cases.items():
            self.set_single_range(low, width, high)
            self.calculate()
            with self.subTest(f"Range validation / {label} is warned about"):
                self.assertNotEqual("", self.validation_text(), f"low={low}, width={width}, high={high} was accepted")
            with self.subTest(f"Range validation / {label} flags the histogram input"):
                self.assertIn("histogram", self.error_indicators())

        # and the warning must clear again, otherwise a stale warning would make every later case
        # above look as though it had been caught
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)
        self.calculate()
        with self.subTest("Range validation / correcting the range clears the warning"):
            self.assertEqual("", self.validation_text())
            self.assertEqual([], self.error_indicators())

    def _check_spin_boxes_refuse_bad_input(self):
        """The guide's 'it should not allow you to input ... (no warning will be printed)' list."""
        spin_box = self.view.single_low_spin_box
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)

        with self.subTest("Range validation / a negative number cannot be entered"):
            _type_into(spin_box, "-5")
            spin_box.interpretText()
            self.assertGreaterEqual(spin_box.value(), 0.0, "the spin box accepted a negative wavelength")

        with self.subTest("Range validation / non-numeric characters cannot be entered"):
            text = _type_into(spin_box, "abc")
            self.assertEqual("", text, f"the spin box accepted the text '{text}'")

        with self.subTest("Range validation / punctuation cannot be entered"):
            # the comma is excluded deliberately: the view converts it to a decimal point on purpose
            # (see SampleTransmissionCalculatorView._double_spinbox_textChanged), so it is the one
            # piece of punctuation the interface is supposed to accept
            text = _type_into(spin_box, "!?;")
            self.assertEqual("", text, f"the spin box accepted the text '{text}'")

        with self.subTest("Range validation / rejecting input prints no warning"):
            self.assertEqual("", self.validation_text())

    def _check_emptied_box_is_restored(self):
        """Guide: 'If you delete the contents of a box and then click Calculate it will reset it
        with the previously entered values.'"""
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)
        spin_box = self.view.single_low_spin_box

        _type_into(spin_box, "")
        self.calculate()

        with self.subTest("Range validation / an emptied box keeps its previous value"):
            self.assertEqual(GUIDE_LOW, spin_box.value())

        with self.subTest("Range validation / an emptied box shows its previous value again"):
            self.assertNotEqual("", spin_box.lineEdit().text())

        with self.subTest("Range validation / the calculation still succeeds"):
            self.assertEqual("", self.validation_text())


class StcGuiMultipleRangeTest(SampleTransmissionGuiTestBase):
    """Guide section 'Multiple Wavelength Range'.

    The bin centres are the whole point of the section - the guide has the tester open the workspace
    and read them off - so they are checked against the workspace rather than against the plot.
    """

    def setUp(self):
        super(StcGuiMultipleRangeTest, self).setUp()
        self.set_sample(VALID_FORMULA, VALID_DENSITY, VALID_THICKNESS)

    def test_multiple_wavelength_range(self):
        self.set_multiple_range("1,1,3")
        self.calculate()
        self.assertEqual("", self.validation_text(), "the guide's own binning string was rejected")

        with self.subTest("Multiple range / steps 2-3 ('1,1,3' gives 2 bins at 1.5 and 2.5 Ang)"):
            self.assertEqual([1.5, 2.5], self.wavelength_bin_centres())

        self.set_multiple_range("1,1,3,0.5,4")
        self.calculate()

        with self.subTest("Multiple range / step 4 ('1,1,3,0.5,4' adds bins at 3.25 and 3.75 Ang)"):
            centres = self.wavelength_bin_centres()
            self.assertEqual([1.5, 2.5, 3.25, 3.75], centres)

        self._check_multiple_range_validation()

    def _check_multiple_range_validation(self):
        """Guide step 5: 'Repeat the validation tests' for the multiple range."""
        cases = {
            "Low > High": "3,1,1",
            "Width > High - Low": "1,10,3",
            "Width = 0": "1,0,3",
            "an even number of values": "1,1",
            "a non-numeric value": "1,one,3",
        }
        for label, binning in cases.items():
            self.set_multiple_range(binning)
            self.calculate()
            with self.subTest(f"Multiple range / step 5 ({label}: '{binning}' is warned about)"):
                self.assertNotEqual("", self.validation_text(), f"'{binning}' was accepted")
                self.assertIn("histogram", self.error_indicators())


class StcGuiChemicalFormulaValidationTest(SampleTransmissionGuiTestBase):
    """Guide section 'Chemical Formula Validation'.

    Every case here must reach the bottom-of-window warning. The guide also says the error comes
    "from CalculateSampleTransmission" - the interface catches that and turns it into the warning,
    which is what makes it observable at all, so the warning is what is asserted.
    """

    # the guide's five cases, plus the blank formula the model validates for itself
    BAD_FORMULAE = ("C2H4", "Z", "0", "*", "2C 4H", "")

    def setUp(self):
        super(StcGuiChemicalFormulaValidationTest, self).setUp()
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)

    def test_chemical_formula_validation(self):
        for formula in self.BAD_FORMULAE:
            set_line_edit(self.view.chemical_formula_line_edit, formula)
            self.view.density_spin_box.setValue(VALID_DENSITY)
            self.view.thickness_spin_box.setValue(VALID_THICKNESS)
            process_events()
            self.calculate()

            with self.subTest(f"Formula validation / '{formula}' produces a warning"):
                self.assertNotEqual("", self.validation_text(), f"'{formula}' was accepted as a chemical formula")

            with self.subTest(f"Formula validation / '{formula}' flags the chemical formula input"):
                self.assertIn("chemical_formula", self.error_indicators())

        # a good formula must clear it again, so the checks above cannot be passing on a warning
        # that was simply never cleared
        set_line_edit(self.view.chemical_formula_line_edit, VALID_FORMULA)
        self.calculate()
        with self.subTest("Formula validation / a valid formula clears the warning"):
            self.assertEqual("", self.validation_text())
            self.assertEqual([], self.error_indicators())


if __name__ == "__main__":
    unittest.main()
