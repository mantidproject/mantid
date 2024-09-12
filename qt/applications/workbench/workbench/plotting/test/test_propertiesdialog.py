# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#


# system imports
import unittest

from unittest.mock import MagicMock

# third-party library imports
import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from matplotlib.ticker import LogFormatterSciNotation, ScalarFormatter
import numpy as np

from mantid.api import WorkspaceFactory
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.plotting.functions import pcolormesh
from workbench.plotting.propertiesdialog import (
    XAxisEditor,
    YAxisEditor,
    ZAxisEditor,
    ColorbarAxisEditor,
    LegendEditor,
    DECIMAL_FORMAT,
    SCIENTIFIC_FORMAT,
)


@start_qapplication
class PropertiesDialogTest(unittest.TestCase):
    # Success tests
    def test_axis_editor_initialised_with_correct_values(self):
        # make figure
        fig, ax = plt.subplots(1, 1)
        ax.plot([1, 2, 3], [1, 10, 100], "o")
        # set properties that can be accessed via the axes menu
        ax.xaxis.grid(True)
        ax.set(xlim=[0, 4], ylim=[1e-3, 1e3], yscale="log")
        # get an AxisEditor object for x/y axes
        xEditor = XAxisEditor(fig.canvas, ax)
        yEditor = YAxisEditor(fig.canvas, ax)
        # test grid visibility
        self.assertEqual(xEditor._memento.grid, ax.xaxis._major_tick_kw["gridOn"])
        self.assertEqual(yEditor._memento.grid, ax.yaxis._major_tick_kw["gridOn"])
        # test limits
        self.assertEqual(xEditor._memento.min, ax.get_xlim()[0])
        self.assertEqual(xEditor._memento.max, ax.get_xlim()[1])
        self.assertEqual(yEditor._memento.min, ax.get_ylim()[0])
        self.assertEqual(yEditor._memento.max, ax.get_ylim()[1])
        # test format
        self.assertEqual(xEditor._memento.formatter, "Decimal Format")
        self.assertEqual(yEditor._memento.formatter, "Scientific Format")
        # test scale
        self.assertEqual(xEditor._memento.log, False)
        self.assertEqual(yEditor._memento.log, True)

    def test_axis_editor_initialised_with_correct_values_3d(self):
        a = np.array([[1]])
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})
        ax.plot_surface(a, a, a)
        # Set properties that can be accessed via the axes menu
        ax.set_xlim(1, 2)
        ax.set_ylim(3, 4)
        ax.set_zlim(5, 6)
        ax.xaxis.set_major_formatter(ScalarFormatter(useOffset=True))
        ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=True))
        ax.zaxis.set_major_formatter(LogFormatterSciNotation())
        # Create axis editors for each axis
        x_editor = XAxisEditor(fig.canvas, ax)
        y_editor = YAxisEditor(fig.canvas, ax)
        z_editor = ZAxisEditor(fig.canvas, ax)
        # Check that the correct axis is assigned
        self.assertEqual(x_editor.axis, ax.xaxis)
        self.assertEqual(y_editor.axis, ax.yaxis)
        self.assertEqual(z_editor.axis, ax.zaxis)
        # Test tick formats
        self.assertEqual(x_editor._memento.formatter, DECIMAL_FORMAT)
        self.assertEqual(y_editor._memento.formatter, DECIMAL_FORMAT)
        self.assertEqual(z_editor._memento.formatter, SCIENTIFIC_FORMAT)
        # Test limits
        self.assertEqual(x_editor._memento.min, ax.get_xlim()[0])
        self.assertEqual(x_editor._memento.max, ax.get_xlim()[1])
        self.assertEqual(y_editor._memento.min, ax.get_ylim()[0])
        self.assertEqual(y_editor._memento.max, ax.get_ylim()[1])
        self.assertEqual(z_editor._memento.min, ax.get_zlim()[0])
        self.assertEqual(z_editor._memento.max, ax.get_zlim()[1])

    def test_changes_apply_to_all_colorfill_plots_if_one_colorbar(self):
        ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=1, YLength=5, XLength=5)
        fig = pcolormesh([ws, ws])
        # there should be 3 axes: 2 colorfill plots and 1 colorbar
        self.assertEqual(3, len(fig.axes))

        colorbarEditor = ColorbarAxisEditor(fig.canvas, fig.axes[2])

        min_value = 1.0
        max_value = 2.0
        colorbarEditor.ui.editor_min.text = MagicMock(return_value=min_value)
        colorbarEditor.ui.editor_max.text = MagicMock(return_value=max_value)
        colorbarEditor.ui.logBox.isChecked = MagicMock(return_value=True)

        colorbarEditor.changes_accepted()

        for ax in range(2):
            self.assertEqual(min_value, fig.axes[ax].collections[0].norm.vmin)
            self.assertEqual(max_value, fig.axes[ax].collections[0].norm.vmax)
            self.assertTrue(isinstance(fig.axes[ax].collections[0].norm, LogNorm))

    def test_legend_editor_correctly_updates_axis_legend(self):
        # make figure
        fig, ax = plt.subplots(1, 1)
        lines = ax.plot([1, 2, 3], [1, 10, 100], "o", label="Old label")
        ax.legend()
        legend_text = ax.get_legend().get_texts()[0]
        # Make the legend editor
        legend_editor = LegendEditor(fig.canvas, legend_text, lines[0])
        legend_editor.ui.editor.text = MagicMock(return_value="New label")

        legend_editor.changes_accepted()

        self.assertEqual(ax.get_legend().get_texts()[0].get_text(), "New label")
        self.assertEqual(lines[0].get_label(), "New label")

    def test_grid_option_correct_when_grid_set_with_kwargs(self):
        """
        Plot script generator toggles grid lines using axes.tick_params. Reproduce that here to check whether the
        grid line visibility is being picked up correctly by the axis editor dialog.
        """
        # make figure
        fig, ax = plt.subplots(1, 1)
        ax.plot([1, 2, 3], [1, 10, 100], "o")
        # set properties that can be accessed via the axes menu
        ax.tick_params(
            axis="x",
            which="major",
            **{
                "gridOn": True,
                "tick1On": True,
                "tick2On": False,
                "label1On": True,
                "label2On": False,
                "size": 6,
                "tickdir": "out",
                "width": 1,
            },
        )
        ax.tick_params(
            axis="y",
            which="major",
            **{
                "gridOn": True,
                "tick1On": True,
                "tick2On": False,
                "label1On": True,
                "label2On": False,
                "size": 6,
                "tickdir": "out",
                "width": 1,
            },
        )

        # get an AxisEditor object for x/y axes
        x_editor = XAxisEditor(fig.canvas, ax)
        y_editor = YAxisEditor(fig.canvas, ax)

        self.assertTrue(x_editor._memento.grid)
        self.assertTrue(y_editor._memento.grid)


if __name__ == "__main__":
    unittest.main()
