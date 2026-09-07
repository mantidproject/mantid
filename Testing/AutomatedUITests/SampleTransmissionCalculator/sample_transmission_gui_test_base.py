# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Sample Transmission Calculator automated UI tests.

These replace ``dev-docs/source/Testing/General/SampleTransmissionCalculatorTestGuide.rst``. The
interface is the simplest in the set - no data files, no facility, no background work, and every
number the guide quotes comes straight out of ``CalculateSampleTransmission`` - so this suite is
also the cheapest end-to-end check that the harness works against a Python MVP interface.

Like ``AutomatedUITestBase`` this module defines no ``test_*`` method and no ``runTest``, which is
what keeps ``unittest``'s loader from collecting it out of the modules that import it.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import click, process_events, select_combo, set_line_edit  # noqa: E402

# the workspace CalculateSampleTransmission leaves in the ADS. Named after the local variable the
# model assigns it to, which is what simpleapi uses when no OutputWorkspace is given - so the guide's
# "right-click on the transmission_ws workspace" is really an assertion about the model's variable
# name, and it is checked here as such.
OUTPUT_WORKSPACE = "transmission_ws"

BINNING_SINGLE = "Single"
BINNING_MULTIPLE = "Multiple"

MASS_DENSITY = "Mass Density"
NUMBER_DENSITY = "Number Density"

# the guide's "Basic Usage" inputs, reused as the starting point of every scenario that then varies
# one thing at a time
GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH = 1.8, 1.0, 7.8


class SampleTransmissionGuiTestBase(AutomatedUITestBase):
    """Builds the Sample Transmission Calculator and exposes its inputs and outputs.

    The interface calculates synchronously on the click - there is no worker to wait for - so unlike
    most suites here nothing in it needs ``wait_for_async_task``.
    """

    def setUp(self):
        super(SampleTransmissionGuiTestBase, self).setUp()
        self.gui = None
        self._build_gui()

    def _build_gui(self):
        # imported here rather than at module scope so a build without the interface skips cleanly
        from mantidqtinterfaces.SampleTransmissionCalculator.stc_gui import SampleTransmissionCalculator

        self.gui = SampleTransmissionCalculator()
        self.gui.show()
        process_events(2)
        self.presenter = self.gui.presenter
        self.view = self.presenter.view

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            self._drain_async_tasks()
            self._clear_ads()
            process_events(2)
            gui.close()
            process_events(2)
            self.gui = None
        super(SampleTransmissionGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ inputs

    def set_single_range(self, low=GUIDE_LOW, width=GUIDE_WIDTH, high=GUIDE_HIGH):
        """Fill in the Single wavelength range.

        Deliberately not routed through ``set_spin_box``: half of what the guide asks is that these
        boxes *refuse* certain values, so a helper that raised on a value the widget clamped would
        turn the observation under test into an error. The clamping is asserted at the call sites
        that care about it instead.
        """
        select_combo(self.view.binning_type_combo_box, BINNING_SINGLE)
        self.view.single_low_spin_box.setValue(low)
        self.view.single_width_spin_box.setValue(width)
        self.view.single_high_spin_box.setValue(high)
        process_events()

    def set_multiple_range(self, binning):
        select_combo(self.view.binning_type_combo_box, BINNING_MULTIPLE)
        set_line_edit(self.view.multiple_line_edit, binning)

    def set_sample(self, chemical_formula, density, thickness, density_type=MASS_DENSITY):
        set_line_edit(self.view.chemical_formula_line_edit, chemical_formula)
        select_combo(self.view.density_combo_box, density_type)
        self.view.density_spin_box.setValue(density)
        self.view.thickness_spin_box.setValue(thickness)
        process_events()

    def calculate(self):
        """Press Calculate. Synchronous - the model runs the algorithm on the GUI thread."""
        click(self.view.calculate_button)
        process_events(2)

    # ------------------------------------------------------------------ outputs

    def transmission(self):
        """The calculated transmission, as the interface plotted it.

        Read from the plot rather than from the workspace on purpose: the guide's checks are about
        what the *interface* showed, and the plotted curve is one step further down the path a user
        takes than the algorithm's output is.
        """
        lines = self.view.axes.get_lines()
        if not lines:
            raise AssertionError("nothing has been plotted; did Calculate succeed?")
        line = lines[0]
        return line.get_xdata(), line.get_ydata()

    def wavelength_bin_centres(self):
        """Bin centres of the output workspace, which is what 'Show Data' shows the tester.

        ``CalculateSampleTransmission`` converts to point data before it returns, so its X values are
        already the centres - the guide's "2 bins at 1.5 Ang and 2.5 Ang" is read straight off them.
        The histogram branch is kept so that a future change back to bin edges is reported as the
        centres moving rather than as every bin silently shifting by half a width.
        """
        from mantid.api import AnalysisDataService as ADS

        workspace = ADS.retrieve(OUTPUT_WORKSPACE)
        x_values = workspace.readX(0)
        if len(x_values) == len(workspace.readY(0)):
            return [float(x) for x in x_values]
        return [0.5 * float(x_values[i] + x_values[i + 1]) for i in range(len(x_values) - 1)]

    def validation_text(self):
        """The red warning line along the bottom of the window; empty when the inputs were good."""
        return self.view.validation_label.text()

    def error_indicators(self):
        """Which inputs are flagged with an asterisk, by the presenter's own names for them."""
        return sorted(
            name for name in ("histogram", "chemical_formula", "density", "thickness") if getattr(self.view, f"{name}_err").text()
        )

    def results_table(self):
        """The results tree as ``{"Scattering": text, "Min": text, ...}``.

        Flattened because the tree is one fixed shape - a Scattering row and a Transmission row whose
        children are the statistics - so the nesting carries no information a test needs.
        """
        tree = self.view.results_tree
        values = {}
        for index in range(tree.topLevelItemCount()):
            item = tree.topLevelItem(index)
            values[item.text(0)] = item.text(1)
            for child_index in range(item.childCount()):
                child = item.child(child_index)
                values[child.text(0)] = child.text(1)
        return values
