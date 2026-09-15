# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the ISIS SANS automated UI tests.

These replace ``dev-docs/source/Testing/SANSGUI/ISISSANSGUITests.rst``.

The interface is a Python MVP triple whose view is built first and handed to the presenter, which is
how ``ISIS_SANS.py`` starts it and how it is built here.

**Data.** The guide works from the Training data download (``loqdemo/MaskFile.toml`` and
``loqdemo/batch_mode_reduction.csv``), which is not part of the repository's ExternalData store, so
it is not available to the weekly run. The suite is split accordingly, per the decision recorded in
``dev-docs/source/AutomatedUITests.rst``:

* the scenarios whose observations are about the *interface* - the save-option matrix, the runs
  table, the user-file revert semantics, the tooltips - use the equivalent LOQ user file and batch
  file that *are* in the SystemTest data, because nothing in them depends on which particular file
  was loaded;
* the reduction scenarios, whose observations are the guide's exact workspace and file counts,
  declare the Training data by name and skip cleanly without it. Those counts are only true for that
  batch file, and asserting them against a different one would be inventing a result.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import process_events  # noqa: E402

# The guide's files, by the names it uses. Declared so that a run without the Training data reports
# a skip against the name the guide tells a tester to look for.
GUIDE_USER_FILE = "MaskFile.toml"
GUIDE_BATCH_FILE = "batch_mode_reduction.csv"

# The equivalents in the repository's SystemTest data, used by the scenarios that only need *a* user
# file and *a* batch file rather than the guide's particular ones.
LOQ_USER_FILE = "MASK_094AA.toml"
LOQ_BATCH_FILE = "loq_batch_mode_reduction.csv"

RUN_TAB = "Runs"


class SansGuiTestBase(AutomatedUITestBase):
    """Builds the ISIS SANS interface and gives access to its runs table."""

    def required_files(self):
        return ()

    def setUp(self):
        super(SansGuiTestBase, self).setUp()
        self.require_files(*self.required_files())
        self.gui = None
        self.output_dir = os.path.join(self.tmp_root, "output")
        os.makedirs(self.output_dir, exist_ok=True)
        self._build_gui()

    def _build_gui(self):
        # imported here rather than at module scope so a build without the interface skips cleanly
        from mantidqtinterfaces.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
        from mantidqtinterfaces.sans_isis.gui_logic.models.run_tab_model import RunTabModel
        from mantidqtinterfaces.sans_isis.gui_logic.presenter.run_tab_presenter import RunTabPresenter
        from sans.common.enums import SANSFacility

        self.gui = SANSDataProcessorGui()
        self.presenter = RunTabPresenter(SANSFacility.ISIS, run_tab_model=RunTabModel(), view=self.gui)
        self.gui.show()
        process_events(3)

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            self._drain_async_tasks()
            self._clear_ads()
            process_events(2)
            gui.close()
            process_events(2)
            self.gui = None
        super(SansGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ files

    @staticmethod
    def resolve(filename):
        from mantid.api import FileFinder

        path = FileFinder.getFullPath(filename)
        if not path:
            raise AssertionError(f"{filename} could not be found on the data search path")
        return path

    def load_user_file(self, filename=LOQ_USER_FILE):
        """Load a user file the way the guide's set-up does.

        The path goes into the same line edit the browse dialog fills in, and the presenter's own
        handler then reads it back out - so everything downstream of the file dialog runs unmodified.
        """
        self.gui.user_file_line_edit.setText(self.resolve(filename))
        self.presenter.on_user_file_load()
        process_events(3)

    def load_batch_file(self, filename=LOQ_BATCH_FILE):
        self.gui.batch_line_edit.setText(self.resolve(filename))
        self.presenter.on_batch_file_load()
        process_events(3)

    # ------------------------------------------------------------------ save options

    def set_reduction_dimensionality(self, dimensionality):
        """Choose 1D or 2D, by clicking the radio button the guide names."""
        from qt_interaction_helpers import click

        button = self.gui.reduction_dimensionality_1D if dimensionality == "1D" else self.gui.reduction_dimensionality_2D
        click(button)
        process_events(2)

    def set_output_mode(self, mode):
        """Choose Memory, File or Both, by clicking the radio button."""
        from qt_interaction_helpers import click

        buttons = {
            "Memory": self.gui.output_mode_memory_radio_button,
            "File": self.gui.output_mode_file_radio_button,
            "Both": self.gui.output_mode_both_radio_button,
        }
        if mode not in buttons:
            raise AssertionError(f"'{mode}' is not an output mode; expected one of {sorted(buttons)}")
        click(buttons[mode])
        process_events(2)

    def save_checkboxes(self):
        """The three save-format check boxes, keyed by the label the guide uses for each."""
        return {
            "CanSAS (1D)": self.gui.can_sas_checkbox,
            "NxCanSAS (1D/2D)": self.gui.nx_can_sas_checkbox,
            "RKH (1D/2D)": self.gui.rkh_checkbox,
        }

    def save_checkbox_states(self):
        return {name: (box.isEnabled(), box.isChecked()) for name, box in self.save_checkboxes().items()}

    # ------------------------------------------------------------------ the runs table

    @property
    def table(self):
        """The runs table itself - a C++ ``JobTreeView``, driven through the view's own accessors
        (``get_cell``, ``add_row``, ``clear_table``) rather than through the ``model_*`` helpers."""
        return self.gui.data_processor_table

    def table_row_count(self):
        return self.presenter._table_model.get_number_of_rows()

    def export_table_to(self, path):
        """Press Export Table and answer its save dialog with ``path``.

        The dialog is the only blocking part of the export; everything after it - which rows are
        written, and by which parser - runs unmodified, which is what the guide's checks are about.
        """
        from unittest import mock

        from qt_interaction_helpers import click

        with mock.patch.object(self.presenter, "display_save_file_box", return_value=path):
            click(self.gui.export_table_button)
            process_events(3)
        return path

    def visible_columns(self):
        """The columns a user can currently see, in table order.

        Which columns are shown is the observable in the guide's "some extra columns should appear"
        steps, and the interface controls it by hiding and showing columns rather than by rebuilding
        the table.
        """
        return [name for index, name in enumerate(self.gui.COLUMN_NAMES) if not self.table.isColumnHidden(index)]
