# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.NotebookView import NotebookView

import unittest
from unittest.mock import MagicMock, patch, call


class TestNotebookView(unittest.TestCase):
    def setUp(self) -> None:
        with patch("instrumentview.NotebookView.pv"):
            self._view = NotebookView()

    def test_add_selection_mesh_calls_pyvista_with_expected_parameters(self):
        cloud = MagicMock(name="cloud")
        scalars = "my_scalars"

        self._view.add_selection_mesh(cloud, scalars)
        self._view._plotter.add_mesh.assert_called_once_with(
            cloud,
            scalars=scalars,
            opacity=[0.0, 0.3],
            show_scalar_bar=False,
            pickable=False,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def test_reset_camera_delegates_to_plotter(self):
        self._view.reset_camera()
        self._view._plotter.reset_camera.assert_called_once()

    def test_show_axes_delegates_to_plotter(self):
        self._view.show_axes()
        self._view._plotter.show_axes.assert_called_once()

    def test_pick_detectors_delegates_to_presenter(self):
        presenter = MagicMock()
        self._view.subscribe_presenter(presenter)

        ids = [1, 2, 3]
        self._view.pick_detectors(ids, sum_spectra=True)
        presenter.pick_detectors.assert_called_once_with(ids, True)

    def _make_fake_workspace(self, num_hists, spectra):
        """
        Build a minimal fake Workspace-like object with the methods used
        by _plot_spectra: getNumberHistograms() and getSpectrumNumbers().
        """
        ws = MagicMock()
        ws.getSpectrumNumbers.return_value = spectra
        return ws

    def test_plot_spectra_summed_does_not_set_labels_or_legend(self):
        ws = self._make_fake_workspace(num_hists=3, spectra=[10, 11, 12])

        with patch("instrumentview.NotebookView.plt.subplots") as subplots:
            axes = MagicMock()
            subplots.return_value = (MagicMock(name="fig"), axes)
            result_axes = self._view.plot_spectra(ws, sum_spectra=True)
            subplots.assert_called_once_with(subplot_kw={"projection": "mantid"})
            # Called once per spectrum, label should be None when sum_spectra=True
            expected_calls = [
                call(ws, specNum=10, label=None),
                call(ws, specNum=11, label=None),
                call(ws, specNum=12, label=None),
            ]
            call_list = axes.plot.call_args_list
            self.assertEqual(len(expected_calls), len(call_list))
            for idx in range(len(expected_calls)):
                self.assertEqual(call_list[idx], expected_calls[idx])
            # No legend when summed
            axes.legend.assert_not_called()
            self.assertEqual(axes, result_axes)

    def test_plot_spectra_not_summed_sets_labels_and_draggable_legend(self):
        ws = self._make_fake_workspace(num_hists=2, spectra=[1, 2])

        with patch("instrumentview.NotebookView.plt.subplots") as subplots:
            axes = MagicMock()
            legend = MagicMock()
            axes.legend.return_value = legend
            subplots.return_value = (MagicMock(name="fig"), axes)
            result_axes = self._view.plot_spectra(ws, sum_spectra=False)
            expected_calls = [
                call(ws, specNum=1, label="Spectrum 1"),
                call(ws, specNum=2, label="Spectrum 2"),
            ]
            call_list = axes.plot.call_args_list
            self.assertEqual(len(expected_calls), len(call_list))
            for idx in range(len(expected_calls)):
                self.assertEqual(call_list[idx], expected_calls[idx])
            # Legend created with fontsize=8.0 and made draggable
            axes.legend.assert_called_once_with(fontsize=8.0)
            legend.set_draggable.assert_called_once_with(True)
            self.assertEqual(axes, result_axes)


if __name__ == "__main__":
    unittest.main()
