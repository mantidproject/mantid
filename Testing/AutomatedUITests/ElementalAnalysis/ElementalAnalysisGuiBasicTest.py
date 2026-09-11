# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Elemental Analysis interface.

Replaces the whole of ``dev-docs/source/Testing/ElementalAnalysis/ElementalAnalysisTests.rst``,
whose "Basic tests" section is four sentences: the window starts up with dummy widgets, it has a
Manage user directories row and a help button along the bottom, its tabs can be undocked and
re-dock when closed, and closing the window closes any undocked tab with it.

That last one is the only observation in the guide with real consequences - an undocked tab is a
top-level window with no parent, so one left behind by a closed interface is a window the user
cannot get rid of - and it is the reason this suite is worth having despite the interface being
hidden from users.
"""

import unittest

from elemental_analysis_gui_test_base import ALL_TABS, TAB_FITTING, TAB_GROUPING, TAB_HOME, ElementalAnalysisGuiTestBase
from qt_interaction_helpers import process_events, top_level_widget_names


class ElementalAnalysisGuiStartupTest(ElementalAnalysisGuiTestBase):
    """Guide: 'When the GUI starts up it will have some dummy widgets ... The bottom of the GUI will
    have a Manage user directories and help buttons.'"""

    def test_startup_layout(self):
        with self.subTest("Basic tests / the interface opens with its four tabs"):
            self.assertEqual(list(ALL_TABS), self.attached_tab_titles())

        with self.subTest("Basic tests / no tab starts out undocked"):
            self.assertEqual([], self.detached_tab_titles())

        with self.subTest("Basic tests / the dummy widgets are present but inert"):
            # the guide's "these do not do anything" is a statement about the interface being
            # unfinished; what is checkable is that Home and Fitting are still the placeholder line
            # edits rather than real tabs, so this observation starts failing the day one is
            # implemented - which is exactly when the guide needs revisiting
            from qtpy.QtWidgets import QLineEdit

            self.assertIsInstance(self.gui.home_tab, QLineEdit)
            self.assertIsInstance(self.gui.fitting_tab, QLineEdit)

        with self.subTest("Basic tests / the bottom of the interface offers Manage User Directories"):
            self.assertTrue(self.help_view.manage_user_dir_button.isVisible())
            self.assertEqual("Manage User Directories", self.help_view.manage_user_dir_button.text())

        with self.subTest("Basic tests / and a help button"):
            self.assertTrue(self.help_view.help_button.isVisible())

        with self.subTest("Basic tests / the plotting panel is docked into the window"):
            self.assertIn(self.gui.dockable_plot_widget_window, self.gui.findChildren(type(self.gui.dockable_plot_widget_window)))


class ElementalAnalysisGuiDockingTest(ElementalAnalysisGuiTestBase):
    """Guide: 'The tabs should be dockable, closing them will result in the tab returning to the
    docked state. Closing the GUI should also close any undocked tabs.'"""

    def test_tabs_can_be_undocked_and_redock_when_closed(self):
        for title in (TAB_GROUPING, TAB_HOME):
            self.detach_tab(title)

            with self.subTest(f"Basic tests / the '{title}' tab can be undocked"):
                self.assertIn(title, self.detached_tab_titles())
                self.assertNotIn(title, self.attached_tab_titles())

            with self.subTest(f"Basic tests / the undocked '{title}' tab is a window of its own"):
                detached = self.tabs.detachedTabs[title]
                self.assertTrue(detached.isVisible())
                self.assertEqual(title, detached.windowTitle())

            self.close_detached_tab(title)

            with self.subTest(f"Basic tests / closing the undocked '{title}' tab re-docks it"):
                self.assertNotIn(title, self.detached_tab_titles())
                self.assertIn(title, self.attached_tab_titles())

        with self.subTest("Basic tests / re-docking restores the original tab order"):
            # the widget tracks tab_order for exactly this, so a tab that went away and came back
            # must not end up at the end of the bar
            self.assertEqual(list(ALL_TABS), self.attached_tab_titles())

    def test_closing_the_interface_closes_undocked_tabs(self):
        self.detach_tab(TAB_GROUPING)
        self.detach_tab(TAB_FITTING)
        detached = [self.tabs.detachedTabs[TAB_GROUPING], self.tabs.detachedTabs[TAB_FITTING]]

        self.assertEqual([TAB_FITTING, TAB_GROUPING], self.detached_tab_titles(), "the tabs were not undocked")

        gui = self.gui
        self.gui = None  # tearDown must not close it twice; the window is WA_DeleteOnClose
        gui.close()
        process_events(3)

        with self.subTest("Basic tests / closing the interface closes every undocked tab"):
            for window in detached:
                self.assertFalse(window.isVisible(), f"'{window.windowTitle()}' outlived the interface")

        with self.subTest("Basic tests / no undocked tab is left among the open windows"):
            names = top_level_widget_names()
            for title in (TAB_GROUPING, TAB_FITTING):
                self.assertNotIn(title, names)


if __name__ == "__main__":
    unittest.main()
