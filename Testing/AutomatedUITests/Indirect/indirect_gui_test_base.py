# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Indirect automated UI tests.

These replace ``dev-docs/source/Testing/Indirect/DiffractionTests.rst`` and
``dev-docs/source/Testing/Indirect/DataReductionTests.rst``.

Both interfaces are C++ ``UserSubWindow``s registered with ``DECLARE_SUBWINDOW``, so there is no
Python view or presenter to reach: the factory hands back the window and nothing else. Every widget
is found by its ``objectName`` from the ``.ui`` files under ``qt/scientific_interfaces/Indirect``,
through ``child_named``, and the names are collected as constants in this module so that a renamed
widget is a one-line fix here rather than a hunt through the tests.

**Data.** Both guides need ISIS archive runs (IRIS 26173/26176/26184-5, OSIRIS 89813/89757), which
the weekly runners do not have. The reduction scenarios therefore declare those runs and skip
cleanly without them. What runs everywhere is the half of each guide that is about the interface's
own input validation - which is where both guides put their explicit negative cases, and which needs
no data at all.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import child_named, process_events, select_combo  # noqa: E402

INTERFACE_DIFFRACTION = "Diffraction"
INTERFACE_DATA_REDUCTION = "Data Reduction"

# Data Reduction's tabs, as its .ui file titles them
TAB_ENERGY_TRANSFER = "ISIS Energy Transfer"
TAB_CALIBRATION = "ISIS Calibration"
TAB_DIAGNOSTICS = "ISIS Diagnostics"
TAB_TRANSMISSION = "Transmission"

# The grouping options the Diffraction interface offers, in the order the combo lists them. The
# guide drives All, Groups, Custom and File by these names.
GROUPING_ALL = "All"
GROUPING_FILE = "File"
GROUPING_GROUPS = "Groups"
GROUPING_CUSTOM = "Custom"

# runs the guides name, declared so a run without the archive skips against the name the guide uses
IRIS_DIFFRACTION_RUN = "IRS26176.raw"
OSIRIS_DIFFRACTION_RUN = "OSI89813.raw"
OSIRIS_VANADIUM_RUN = "osi89757.raw"
OSIRIS_CAL_FILE = "osiris_041_RES10.cal"

# The file finders search on a thread pool, and a raw run is looked for across every data search
# directory, so the wait is longer than the ten second default the helpers use.
FILE_SEARCH_TIMEOUT = 60.0


class IndirectGuiTestBase(AutomatedUITestBase):
    """Opens one of the Indirect C++ interfaces and finds its widgets by name.

    Subclasses set ``INTERFACE`` and, where they need data, override ``required_files``.
    """

    INTERFACE = None

    def required_files(self):
        return ()

    def setUp(self):
        super(IndirectGuiTestBase, self).setUp()
        self.require_files(*self.required_files())
        # these interfaces raise their errors as modal message boxes from C++, which no Python patch
        # can intercept; the sweep is what keeps one from hanging the run
        self.dismiss_modal_dialogs()
        self.window = self.open_cpp_interface(self.INTERFACE)
        process_events(2)

    # ------------------------------------------------------------------ widgets

    def widget(self, object_name, widget_type=None, parent=None):
        """A widget of this interface, by the ``objectName`` its .ui file gives it."""
        return child_named(self.window if parent is None else parent, object_name, widget_type)

    def combo(self, object_name, parent=None):
        from qtpy.QtWidgets import QComboBox

        return self.widget(object_name, QComboBox, parent)

    def line_edit(self, object_name, parent=None):
        from qtpy.QtWidgets import QLineEdit

        return self.widget(object_name, QLineEdit, parent)

    def spin_box(self, object_name, parent=None):
        from qtpy.QtWidgets import QSpinBox

        return self.widget(object_name, QSpinBox, parent)

    def label(self, object_name, parent=None):
        from qtpy.QtWidgets import QLabel

        return self.widget(object_name, QLabel, parent)

    def button(self, object_name, parent=None):
        from qtpy.QtWidgets import QPushButton

        return self.widget(object_name, QPushButton, parent)

    def tab_widget(self, object_name):
        from qtpy.QtWidgets import QTabWidget

        return self.widget(object_name, QTabWidget)

    def tab_titles(self, object_name):
        tabs = self.tab_widget(object_name)
        return [tabs.tabText(index) for index in range(tabs.count())]

    def show_tab(self, tab_widget_name, title):
        """Make one of the interface's tabs current, by its title."""
        from qt_interaction_helpers import select_tab

        return select_tab(self.tab_widget(tab_widget_name), title)

    # ------------------------------------------------------------------ common controls

    def instrument_selector(self):
        """The instrument combo.

        Found by type rather than by name: it is a ``MantidWidgets::InstrumentSelector``, added in
        code rather than in the .ui file, so it carries no ``objectName`` to look it up by. It is the
        only combo on these interfaces whose entries are instrument names, which is what identifies
        it here.
        """
        from qtpy.QtWidgets import QComboBox

        for combo in self.window.findChildren(QComboBox):
            entries = [combo.itemText(index) for index in range(combo.count())]
            if any(entry in ("IRIS", "OSIRIS", "TOSCA", "TFXA", "VESUVIO") for entry in entries):
                return combo
        raise AssertionError("no instrument selector was found on this interface")

    def set_instrument(self, instrument):
        select_combo(self.instrument_selector(), instrument)
        # the analyser and reflection combos are repopulated from the instrument definition, which
        # takes a beat
        process_events(5)

    def set_reflection(self, reflection):
        select_combo(self.combo("cbReflection"), reflection)
        process_events(5)

    # ------------------------------------------------------------------ file inputs

    def finder(self, object_name):
        """One of the interface's file finders, by ``objectName``."""
        return self.widget(object_name)

    def set_finder(self, object_name, text):
        """Type a run or a file name into one of the interface's file finders and wait for it.

        Not ``set_finder_text``, which is the helper for a ``FileFinderWidget``: these finders were
        built in C++, and sip hands Python a plain ``QWidget`` for them rather than the wrapped
        class, so ``getText``/``isSearching``/``isValid`` are not reachable. What *is* reachable is
        the ``QLineEdit`` named ``fileEditor`` inside each one - which is the box a user types into,
        so driving it runs the same search.

        The search itself is on a thread pool, and the widget shows a ``valid`` label beside the box
        until it resolves, so that label is what the wait watches.
        """
        from qtpy.QtWidgets import QLabel, QLineEdit
        from qt_interaction_helpers import set_line_edit, wait_until

        finder = self.finder(object_name)
        set_line_edit(child_named(finder, "fileEditor", QLineEdit), text, expect_valid=False)
        valid_label = finder.findChild(QLabel, "valid")
        if valid_label is not None:
            wait_until(
                lambda: not valid_label.isVisible(),
                timeout=FILE_SEARCH_TIMEOUT,
                msg=f"'{object_name}' to resolve '{text}'",
            )
        process_events(3)
        return finder

    def run_is_allowed(self):
        """Whether the interface would let the user press Run.

        The Indirect interfaces express "these inputs are not acceptable" by disabling Run and
        showing a red asterisk beside the offending field, rather than by popping a dialog, so this
        is what every one of the guides' negative cases is really asking about.
        """
        return self.button("pbRun").isEnabled()
