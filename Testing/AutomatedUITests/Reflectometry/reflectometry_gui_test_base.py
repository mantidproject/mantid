# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the ISIS Reflectometry automated UI tests.

These replace the parts of ``dev-docs/source/Testing/ReflectometryGUI/ReflectometryGUITests.rst``
that need neither the ISIS data archive nor the ICAT catalogue.

The interface is a C++ ``UserSubWindow``, so everything is reached by ``objectName`` from the ``.ui``
files under ``qt/scientific_interfaces/ISISReflectometry``. It is unusual among them in having two
levels of tab: ``mainTabs`` holds one page per *batch*, and each batch page has its own ``batchTabs``
with the Runs, Settings, Preview and Save tabs on it. Anything a test looks for therefore has to be
scoped to a batch, because the same widget name exists once per batch - which is why
``batch_widget`` below takes the batch page rather than searching the whole window.

The guide changes the default instrument in scenario 3, and that is process-wide Mantid
configuration rather than a Qt setting, so it goes through ``config_settings`` and is put back.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import child_named, process_events  # noqa: E402

INTERFACE = "ISIS Reflectometry"

MAIN_TABS = "mainTabs"
BATCH_TABS = "batchTabs"

# the tabs each batch offers, as its .ui file titles them
TAB_RUNS = "Runs"
TAB_EVENT_HANDLING = "Event Handling"
TAB_EXPERIMENT_SETTINGS = "Experiment Settings"
TAB_INSTRUMENT_SETTINGS = "Instrument Settings"
TAB_REDUCTION_PREVIEW = "Reduction Preview"
TAB_PLOTTING = "Plotting"
TAB_SAVE = "Save"

BATCH_TAB_TITLES = (
    TAB_RUNS,
    TAB_EVENT_HANDLING,
    TAB_EXPERIMENT_SETTINGS,
    TAB_INSTRUMENT_SETTINGS,
    TAB_REDUCTION_PREVIEW,
    TAB_PLOTTING,
    TAB_SAVE,
)

# the polarisation correction options the guide's scenario 7 works through
POL_CORR_NONE = "None"
POL_CORR_PARAMETER_FILE = "ParameterFile"
POL_CORR_WORKSPACE = "Workspace"
POL_CORR_FILE_PATH = "FilePath"


class ReflectometryGuiTestBase(AutomatedUITestBase):
    """Opens the ISIS Reflectometry interface and scopes widget lookups to a batch."""

    def setUp(self):
        super(ReflectometryGuiTestBase, self).setUp()
        # the interface reports refusals in modal message boxes raised from C++, which no Python
        # patch can intercept; the sweep is what keeps one from hanging the run
        self.dismiss_modal_dialogs()
        self.window = self.open_cpp_interface(INTERFACE)
        process_events(3)

    # ------------------------------------------------------------------ tabs

    def main_tabs(self):
        from qtpy.QtWidgets import QTabWidget

        return child_named(self.window, MAIN_TABS, QTabWidget)

    def batch_titles(self):
        tabs = self.main_tabs()
        return [tabs.tabText(index) for index in range(tabs.count())]

    def batch(self, index=0):
        """The page for one batch, which is what every other lookup is scoped to."""
        tabs = self.main_tabs()
        if index >= tabs.count():
            raise AssertionError(f"there is no batch {index}; the interface has {tabs.count()}")
        tabs.setCurrentIndex(index)
        process_events(2)
        return tabs.widget(index)

    def batch_tab_titles(self, batch=None):
        from qtpy.QtWidgets import QTabWidget

        page = self.batch() if batch is None else batch
        tabs = child_named(page, BATCH_TABS, QTabWidget)
        return [tabs.tabText(index) for index in range(tabs.count())]

    def show_batch_tab(self, title, batch=None):
        from qtpy.QtWidgets import QTabWidget
        from qt_interaction_helpers import select_tab

        page = self.batch() if batch is None else batch
        return select_tab(child_named(page, BATCH_TABS, QTabWidget), title)

    # ------------------------------------------------------------------ widgets

    def batch_widget(self, object_name, widget_type=None, batch=None):
        """A widget belonging to one batch, by ``objectName``.

        Always scoped to a batch page: every one of these names exists once per batch, so a search
        of the whole window would find whichever batch Qt happened to build first.
        """
        return child_named(self.batch() if batch is None else batch, object_name, widget_type)

    def combo(self, object_name, batch=None):
        from qtpy.QtWidgets import QComboBox

        return self.batch_widget(object_name, QComboBox, batch)

    def checkbox(self, object_name, batch=None):
        from qtpy.QtWidgets import QCheckBox

        return self.batch_widget(object_name, QCheckBox, batch)

    def button(self, object_name, batch=None):
        from qtpy.QtWidgets import QPushButton

        return self.batch_widget(object_name, QPushButton, batch)
