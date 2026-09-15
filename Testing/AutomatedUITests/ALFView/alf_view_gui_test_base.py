# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the ALFView automated UI tests.

These replace ``dev-docs/source/Testing/Direct/ALFViewTests.rst``.

ALFView is a C++ ``UserSubWindow``, so it is opened through ``InterfaceManager`` and driven by
``objectName``. It has two tabs - Render and Pick - and its instrument view is a Mantid C++ widget
whose tube picking is mouse-driven.

**Data.** Every one of the guide's scenarios loads ALF run 82301 from the ISIS archive, which the
weekly runners do not have, so the suite declares it and skips cleanly without it. The numbers the
guide checks (two theta 40.4584, peak centre 0.877092, rotation angle 1.26829) are real fitted
results, so fabricating a run in their place would make them meaningless.

The guide's scenario 6 - close ALFView, close Mantid, reopen, and find the vanadium run remembered -
cannot run in one process at all. What is checked instead is that the run is written to the settings
store the interface reads on construction, which is the mechanism that step is testing; the gap is
recorded in FUTURE_WORK.md.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import child_named, process_events  # noqa: E402

INTERFACE = "ALFView"

TAB_RENDER = "Render"
TAB_PICK = "Pick"

# the run the guide loads as both sample and vanadium
ALF_RUN = "ALF82301"

# ALF is an ISIS Excitations instrument, and the interface will not resolve a run without both
FACILITY = "ISIS"
INSTRUMENT = "ALF"


class AlfViewGuiTestBase(AutomatedUITestBase):
    """Opens ALFView and finds its widgets by name."""

    def required_files(self):
        return ()

    def setUp(self):
        super(AlfViewGuiTestBase, self).setUp()
        self.require_files(*self.required_files())

        # the facility and instrument have to be right before the interface is built, and they are
        # process-wide Mantid configuration rather than Qt settings, so they are restored explicitly
        config = self.config_settings(default_facility=FACILITY, default_instrument=INSTRUMENT)
        config.__enter__()
        self.addCleanup(config.__exit__, None, None, None)

        # ALFView reports refusals in modal message boxes raised from C++, which no Python patch can
        # intercept; the sweep is what keeps one from hanging the run
        self.dismiss_modal_dialogs()

        self.window = self.open_cpp_interface(INTERFACE)
        process_events(3)

    # ------------------------------------------------------------------ widgets

    def widget(self, object_name, widget_type=None, parent=None):
        return child_named(self.window if parent is None else parent, object_name, widget_type)

    def tabs(self):
        """ALFView's tab widget, which carries no ``objectName``, so it is found by type.

        There is exactly one ``QTabWidget`` in the interface, which is what makes this safe; if a
        second is ever added this has to become a lookup by name.
        """
        from qtpy.QtWidgets import QTabWidget

        found = self.window.findChildren(QTabWidget)
        if len(found) != 1:
            raise AssertionError(f"expected exactly one tab widget in ALFView, found {len(found)}")
        return found[0]

    def tab_titles(self):
        tabs = self.tabs()
        return [tabs.tabText(index) for index in range(tabs.count())]

    def show_tab(self, title):
        from qt_interaction_helpers import select_tab

        return select_tab(self.tabs(), title)
