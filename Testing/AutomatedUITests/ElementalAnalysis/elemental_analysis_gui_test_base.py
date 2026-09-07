# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Elemental Analysis automated UI tests.

These replace ``dev-docs/source/Testing/ElementalAnalysis/ElementalAnalysisTests.rst``.

Two things about that guide shape this suite.

The first is *which* interface it means. There are two Elemental Analysis GUIs in the tree and both
export a class called ``ElementalAnalysisGui``. The one registered in the Interfaces menu is
``Muon.GUI.ElementalAnalysis``, but the guide describes dummy widgets, a "Manage user directories"
row along the bottom and dockable tabs - none of which that one has, and all of which
``Muon.GUI.ElementalAnalysis2`` does (``setup_dummy``, ``HelpWidget``, ``DetachableTabWidget``). The
guide's own "please note that it is currently hidden from users" says the same thing: it is the
unregistered one. This suite therefore drives ElementalAnalysis2.

The second is that the guide asks for no data. It says data is needed in the set-up, but not one of
the steps under "Basic tests" loads anything - they are all about the window itself - so nothing
here declares required files, and the suite runs everywhere.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import process_events  # noqa: E402

# tabs as ElementalAnalysisGui.setup_tabs adds them
TAB_HOME = "Home"
TAB_GROUPING = "Grouping"
TAB_AUTOMATIC = "Automatic"
TAB_FITTING = "Fitting"
ALL_TABS = (TAB_HOME, TAB_GROUPING, TAB_AUTOMATIC, TAB_FITTING)


class ElementalAnalysisGuiTestBase(AutomatedUITestBase):
    """Builds the Elemental Analysis interface and exposes its tab bar."""

    def setUp(self):
        super(ElementalAnalysisGuiTestBase, self).setUp()
        self.gui = None
        self._build_gui()

    def _build_gui(self):
        # imported here rather than at module scope so a build without the interface skips cleanly
        from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.elemental_analysis import ElementalAnalysisGui

        self.gui = ElementalAnalysisGui()
        self.gui.show()
        process_events(2)
        self.tabs = self.gui.tabs
        self.help_view = self.gui.help_widget.view

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            self._drain_async_tasks()
            # the context holds an ADS observer that repopulates the grouping table on a clear, so
            # the clear has to happen while the interface is still alive to receive it
            self._clear_ads()
            process_events(2)
            # the window is WA_DeleteOnClose, so the reference must not be touched after this
            gui.close()
            process_events(2)
            self.gui = None
        super(ElementalAnalysisGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ tabs

    def tab_index(self, title):
        for index in range(self.tabs.count()):
            if self.tabs.tabText(index) == title:
                return index
        raise AssertionError(f"no tab titled '{title}'; found {[self.tabs.tabText(i) for i in range(self.tabs.count())]}")

    def attached_tab_titles(self):
        return [self.tabs.tabText(index) for index in range(self.tabs.count())]

    def detached_tab_titles(self):
        return sorted(self.tabs.detachedTabs)

    def detach_tab(self, title):
        """Undock a tab the way a user does: a double click on its label in the tab bar.

        ``DetachableTabWidget.TabBar.mouseDoubleClickEvent`` is what raises ``onDetachTabSignal``, so
        going through the signal directly would skip the part most likely to break - that the double
        click reaches the right tab.
        """
        from qtpy.QtCore import Qt
        from qtpy.QtTest import QTest

        tab_bar = self.tabs.tabBar()
        index = self.tab_index(title)
        QTest.mouseDClick(tab_bar, Qt.LeftButton, Qt.NoModifier, tab_bar.tabRect(index).center())
        process_events(3)
        return self.tabs.detachedTabs.get(title)

    def close_detached_tab(self, title):
        """Close an undocked tab's window, which is what re-docks it."""
        detached = self.tabs.detachedTabs.get(title)
        if detached is None:
            raise AssertionError(f"'{title}' is not detached; detached tabs are {self.detached_tab_titles()}")
        detached.close()
        process_events(3)
