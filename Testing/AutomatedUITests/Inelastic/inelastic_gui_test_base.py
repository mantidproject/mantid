# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Inelastic automated UI tests.

These replace the four guides under ``dev-docs/source/Testing/Inelastic/``:
``DataProcessorTests.rst``, ``CorrectionsTests.rst``, ``QENSFittingTests.rst`` and
``BayesFittingTests.rst`` - one interface each.

All four are C++ ``UserSubWindow``s, reached through ``InterfaceManager().createSubWindow`` and
driven by ``objectName`` from the ``.ui`` files under ``qt/scientific_interfaces/Inelastic``.

**Two things constrain what these suites can do, and both are worth knowing before adding to them.**

*Modal errors.* When one of these interfaces refuses a reduction or a fit it says so with a
``QMessageBox`` raised from C++. ``patch_error_messages`` and ``patch_confirmation_box`` both work by
replacing a Python-side symbol, so neither can intercept it, and an unattended test that provokes
one hangs until it is killed. Every test here therefore drives valid inputs only; the guides'
negative cases are recorded in FUTURE_WORK.md.

*Data.* The guides work from the ISIS sample and usage data sets. Some of those files are in the
repository's data store and some are not, so each test declares the ones it needs by name and skips
cleanly without them.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import child_named, click, process_events, select_combo, wait_until  # noqa: E402

INTERFACE_DATA_PROCESSOR = "Data Processor"
INTERFACE_CORRECTIONS = "Corrections"
INTERFACE_QENS_FITTING = "QENS Fitting"
INTERFACE_BAYES_FITTING = "Bayes Fitting"

# Data Processor tabs, as its .ui file titles them. The guide writes the second one "S(Q, w)" too.
TAB_SYMMETRISE = "Symmetrise"
TAB_SQW = "S(Q, w)"
TAB_MOMENTS = "Moments"
TAB_ELWIN = "Elwin"
TAB_IQT = "Iqt"

# Corrections tabs
TAB_CONTAINER_SUBTRACTION = "Container Subtraction"
TAB_ABSORPTION = "Absorption Corrections"
TAB_APPLY_ABSORPTION = "Apply Absorption Corrections"

# QENS Fitting tabs
TAB_MSD = "MSD"
TAB_IQT_FIT = "I(Q, t)"
TAB_CONVOLUTION = "Convolution"
TAB_FUNCTION_Q = "Function (Q)"

# Bayes Fitting tabs
TAB_RESNORM = "ResNorm"
TAB_QUASI = "Quasi"
TAB_STRETCH = "Stretch"

# tab widget object names
TABS_DATA_PROCESSOR = "twIDRTabs"
TABS_CORRECTIONS = "twTabs"
TABS_QENS = "twIDATabs"
TABS_BAYES = "bayesFittingTabs"

# data the guides name
IRIS_REDUCED = "irs26176_graphite002_red.nxs"
IRIS_RESOLUTION = "irs26173_graphite002_res.nxs"
IRIS_CONTAINER = "irs26174_graphite002_red.nxs"

# A reduction or a fit runs on a background thread; these interfaces expose no AsyncTask, so the Run
# button going down and coming back up is what is waited on. See ``press_run`` - which must NOT be
# called ``run``: ``TestCase.run`` is what the framework calls to run the test, and shadowing it
# hands the helper a TestResult as its first argument.
RUN_TIMEOUT = 300.0
STARTED_TIMEOUT = 15.0


class InelasticGuiTestBase(AutomatedUITestBase):
    """Opens one of the Inelastic C++ interfaces and finds its widgets by name.

    Subclasses set ``INTERFACE`` and override ``required_files``.
    """

    INTERFACE = None
    TABS = None

    def required_files(self):
        return ()

    def setUp(self):
        super(InelasticGuiTestBase, self).setUp()
        self.require_files(*self.required_files())
        # these interfaces raise their errors as modal message boxes from C++, which no Python patch
        # can intercept; the sweep is what keeps one from hanging the run
        self.dismiss_modal_dialogs()
        self.window = self.open_cpp_interface(self.INTERFACE)
        process_events(2)

    # ------------------------------------------------------------------ widgets

    def widget(self, object_name, widget_type=None, parent=None):
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

    def button(self, object_name, parent=None):
        from qtpy.QtWidgets import QPushButton

        return self.widget(object_name, QPushButton, parent)

    def tab_titles(self):
        from qtpy.QtWidgets import QTabWidget

        tabs = self.widget(self.TABS, QTabWidget)
        return [tabs.tabText(index) for index in range(tabs.count())]

    def show_tab(self, title):
        from qtpy.QtWidgets import QTabWidget
        from qt_interaction_helpers import select_tab

        return select_tab(self.widget(self.TABS, QTabWidget), title)

    # ------------------------------------------------------------------ input files

    def load_file(self, page, filename):
        """Choose 'File' as the input type and give the file finder a name.

        The finders are ``FileFinderWidget``s built in C++, and sip hands Python a plain ``QWidget``
        for them, so the ``QLineEdit`` named ``fileEditor`` inside each one is what is typed into -
        the same box a user types into. See ``IndirectGuiTestBase.set_finder`` for the longer note.
        """
        from mantid.api import FileFinder
        from qtpy.QtWidgets import QLabel, QLineEdit
        from qt_interaction_helpers import set_line_edit

        path = FileFinder.getFullPath(filename)
        if not path:
            raise AssertionError(f"{filename} could not be found on the data search path")

        select_combo(self.combo("cbInputType", page), "File")
        process_events(2)
        finder = self.widget("dsInput", parent=page) if page.findChild(QLineEdit, "fileEditor") is None else page
        editor = child_named(finder, "fileEditor", QLineEdit)
        set_line_edit(editor, path, expect_valid=False)
        valid_label = editor.parent().findChild(QLabel, "valid")
        if valid_label is not None:
            wait_until(lambda: not valid_label.isVisible(), timeout=60.0, msg=f"the file finder to resolve {filename}")
        process_events(3)

    # ------------------------------------------------------------------ running

    def press_run(self, page=None, timeout=RUN_TIMEOUT):
        """Press Run and wait for it, returning the workspaces it left in the ADS.

        Two phases, and the first may legitimately time out. A job that actually starts disables Run
        for its duration, so waiting for the button to go *down* is how a started job is told from a
        refused one, and waiting for it to come back up is how the started one is waited on. The ADS
        is cleared first because these interfaces reuse one output name per input, so "the
        workspaces changed" is not observable on a second run of the same data.
        """
        from mantid.api import AnalysisDataService as ADS

        ADS.clear()
        run_button = self.button("pbRun", page)
        click(run_button)
        try:
            wait_until(lambda: not run_button.isEnabled(), timeout=STARTED_TIMEOUT, msg="the job to start")
        except RuntimeError:
            process_events(3)
            return sorted(ADS.getObjectNames())
        wait_until(run_button.isEnabled, timeout=timeout, msg="the job to finish")
        process_events(3)
        return sorted(ADS.getObjectNames())
