# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import MagicMock, patch
import matplotlib
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_plot import DNSScPlot


class DNSScPlotTest(unittest.TestCase):
    def setUp(self):
        self.mock_fig = MagicMock()
        self.mock_ax = MagicMock()
        self.mock_fig.add_subplot.return_value = self.mock_ax
        patcher_subplot = patch(
            "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_plot.Subplot", return_value=MagicMock()
        )
        self.addCleanup(patcher_subplot.stop)
        patcher_subplot.start()
        self.plot = DNSScPlot(parent=None, figure=self.mock_fig, grid_helper=None)

    @staticmethod
    def test_set_fontsize():
        with patch.object(matplotlib.rcParams, "update") as mock_update:
            DNSScPlot.set_fontsize(12)
            mock_update.assert_called_once_with({"font.size": 12})

    def test_create_colorbar(self):
        self.plot._plot = MagicMock()
        self.mock_fig.colorbar.return_value = "colorbar"
        self.plot.create_colorbar()
        self.assertEqual(self.plot._colorbar, "colorbar")
        self.mock_fig.colorbar.assert_called_once_with(self.plot._plot, ax=self.plot._ax, extend="max", pad=0.01)

    def test_set_norm(self):
        mock_norm = MagicMock()
        self.plot._plot = MagicMock()
        self.plot.set_norm(mock_norm)
        self.plot._plot.set_norm.assert_called_once_with(mock_norm)

    def test_set_shading(self):
        self.plot._plot = MagicMock()
        self.plot.set_shading("flat")
        self.plot._plot.set_shading.assert_called_once_with("flat")
        self.plot._plot = None
        self.plot.set_shading("gouraud")

    def test_set_grid(self):
        self.plot.set_grid(major=False, minor=True)
        self.plot._ax.xaxis.set_minor_locator.assert_called()
        self.plot.set_grid(major=True, minor=False)
        self.plot._ax.grid.assert_called()

    def test_set_zlim(self):
        self.plot._plot = MagicMock()
        self.plot._colorbar = MagicMock()
        self.plot.set_zlim((1, 5))
        self.plot._plot.set_clim.assert_called_once_with(1, 5)
        self.plot._colorbar.mappable.set_clim.assert_called_once_with(vmin=1, vmax=5)

    def test_set_cmap_calls_set_cmap_on_plot(self):
        self.plot._plot = MagicMock()
        self.plot.set_cmap("viridis")
        self.plot._plot.set_cmap.assert_called_once_with("viridis")

    def test_set_axis_labels(self):
        self.plot.set_axis_labels("x", "y")
        self.plot._ax.set_xlabel.assert_called_once_with("x")
        self.plot._ax.set_ylabel.assert_called_once_with("y")

    def test_plot_triangulation_non_flat(self):
        self.plot._ax.tripcolor.return_value = "plotobj"
        self.plot.plot_triangulation("triang", [0, 1, 2], [1, 2, 3], "cmap", "edges", "shade")
        self.plot._ax.set_visible.assert_called_with(True)
        self.plot._ax.tripcolor.assert_called_once_with("triang", [0, 1, 2], cmap="cmap", edgecolors="edges", shading="shade")
        self.assertEqual(self.plot._plot, "plotobj")

    def test_plot_triangulation_flat(self):
        self.plot._ax.tripcolor.return_value = "plotobj"
        self.plot.plot_triangulation("triang", [0, 1, 2], [1, 2, 3], "cmap", "edges", "flat")
        self.plot._ax.set_visible.assert_called_with(True)
        self.plot._ax.tripcolor.assert_called_once_with("triang", facecolors=[1, 2, 3], cmap="cmap", edgecolors="edges", shading="flat")
        self.assertEqual(self.plot._plot, "plotobj")


if __name__ == "__main__":
    unittest.main()
