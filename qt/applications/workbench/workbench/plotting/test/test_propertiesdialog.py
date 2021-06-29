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

# third-party library imports
import matplotlib

matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from unittest.mock import MagicMock

from mantid.api import WorkspaceFactory
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.plotting.functions import pcolormesh
from workbench.plotting.propertiesdialog import XAxisEditor, YAxisEditor, ColorbarAxisEditor, LegendEditor


@start_qapplication
class PropertiesDialogTest(unittest.TestCase):

    # Success tests
    def test_axis_editor_initialised_with_correct_values(self):
        # make figure
        fig, ax = plt.subplots(1, 1)
        ax.plot([1, 2, 3], [1, 10, 100], 'o')
        # set properties that can be accessed via the axes menu
        ax.xaxis.grid(True)
        ax.set(xlim=[0, 4], ylim=[1e-3, 1e3], yscale='log')
        # get an AxisEditor object for x/y axes
        xEditor = XAxisEditor(fig.canvas, ax)
        yEditor = YAxisEditor(fig.canvas, ax)
        # test grid visibility
        self.assertEqual(xEditor._memento.grid, ax.xaxis._gridOnMajor)
        self.assertEqual(yEditor._memento.grid, ax.yaxis._gridOnMajor)
        # test limits
        self.assertEqual(xEditor._memento.min, ax.get_xlim()[0])
        self.assertEqual(xEditor._memento.max, ax.get_xlim()[1])
        self.assertEqual(yEditor._memento.min, ax.get_ylim()[0])
        self.assertEqual(yEditor._memento.max, ax.get_ylim()[1])
        # test format
        self.assertEqual(xEditor._memento.formatter, 'Decimal Format')
        self.assertEqual(yEditor._memento.formatter, 'Scientific Format')
        # test scale
        self.assertEqual(xEditor._memento.log, False)
        self.assertEqual(yEditor._memento.log, True)

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
        lines = ax.plot([1, 2, 3], [1, 10, 100], 'o', label="Old label")
        ax.legend()
        legend_text = ax.get_legend().get_texts()[0]
        # Make the legend editor
        legend_editor = LegendEditor(fig.canvas, legend_text, lines[0])
        legend_editor.ui.editor.text = MagicMock(return_value="New label")

        legend_editor.changes_accepted()

        self.assertEqual(ax.get_legend().get_texts()[0].get_text(), "New label")
        self.assertEqual(lines[0].get_label(), "New label")


if __name__ == '__main__':
    unittest.main()
