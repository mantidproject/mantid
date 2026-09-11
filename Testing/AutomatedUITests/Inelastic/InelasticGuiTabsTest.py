# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the structure of the four Inelastic interfaces.

Each of the four Inelastic guides opens its interface and works through its tabs in order, so the
first thing every one of them establishes is that those tabs are there and can be selected. That is
what this module checks, for all four interfaces at once.

It needs no data, so it is what these suites contribute to the weekly run on a machine with no ISIS
sample data. The scenarios that reduce and fit are in the other modules in this directory.
"""

import unittest

from inelastic_gui_test_base import (
    INTERFACE_BAYES_FITTING,
    INTERFACE_CORRECTIONS,
    INTERFACE_DATA_PROCESSOR,
    INTERFACE_QENS_FITTING,
    TAB_ABSORPTION,
    TAB_APPLY_ABSORPTION,
    TAB_CONTAINER_SUBTRACTION,
    TAB_CONVOLUTION,
    TAB_ELWIN,
    TAB_FUNCTION_Q,
    TAB_IQT,
    TAB_IQT_FIT,
    TAB_MOMENTS,
    TAB_MSD,
    TAB_QUASI,
    TAB_RESNORM,
    TAB_SQW,
    TAB_STRETCH,
    TAB_SYMMETRISE,
    TABS_CORRECTIONS,
    TABS_BAYES,
    TABS_DATA_PROCESSOR,
    TABS_QENS,
    InelasticGuiTestBase,
)


class _TabChecklist:
    """Asserts an interface's tabs are the ones its guide walks through.

    A mixin rather than a base class: a ``TestCase`` subclass holding these methods would be
    collected and run in its own right, with no interface to open.
    """

    EXPECTED_TABS = ()

    def test_tabs(self):
        with self.subTest(f"{self.INTERFACE} / the interface offers the tabs the guide works through"):
            self.assertEqual(list(self.EXPECTED_TABS), self.tab_titles())

        for title in self.EXPECTED_TABS:
            with self.subTest(f"{self.INTERFACE} / the '{title}' tab can be selected"):
                self.assertTrue(self.show_tab(title).isVisible())


class InelasticGuiDataProcessorTabsTest(_TabChecklist, InelasticGuiTestBase):
    """``Inelastic/DataProcessorTests.rst`` - five tabs."""

    INTERFACE = INTERFACE_DATA_PROCESSOR
    TABS = TABS_DATA_PROCESSOR
    EXPECTED_TABS = (TAB_SYMMETRISE, TAB_SQW, TAB_MOMENTS, TAB_ELWIN, TAB_IQT)


class InelasticGuiCorrectionsTabsTest(_TabChecklist, InelasticGuiTestBase):
    """``Inelastic/CorrectionsTests.rst`` - three tabs."""

    INTERFACE = INTERFACE_CORRECTIONS
    TABS = TABS_CORRECTIONS
    EXPECTED_TABS = (TAB_CONTAINER_SUBTRACTION, TAB_ABSORPTION, TAB_APPLY_ABSORPTION)


class InelasticGuiQensFittingTabsTest(_TabChecklist, InelasticGuiTestBase):
    """``Inelastic/QENSFittingTests.rst`` - four tabs."""

    INTERFACE = INTERFACE_QENS_FITTING
    TABS = TABS_QENS
    EXPECTED_TABS = (TAB_MSD, TAB_IQT_FIT, TAB_CONVOLUTION, TAB_FUNCTION_Q)


class InelasticGuiBayesFittingTabsTest(_TabChecklist, InelasticGuiTestBase):
    """``Inelastic/BayesFittingTests.rst`` - three tabs."""

    INTERFACE = INTERFACE_BAYES_FITTING
    TABS = TABS_BAYES
    EXPECTED_TABS = (TAB_RESNORM, TAB_QUASI, TAB_STRETCH)


if __name__ == "__main__":
    unittest.main()
