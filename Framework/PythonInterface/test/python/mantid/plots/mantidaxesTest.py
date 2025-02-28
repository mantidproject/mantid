# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib
from matplotlib.backend_bases import MouseEvent

matplotlib.use("AGG")
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize
from matplotlib.container import ErrorbarContainer
import numpy as np
import unittest

from mantid.kernel import config
from mantid.plots import datafunctions
from mantid.plots.utility import convert_color_to_hex, MantidAxType
from mantid.plots.axesfunctions import get_colorplot_extents
from unittest.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, DeleteWorkspace, RemoveSpectra, AnalysisDataService as ADS
from mantidqt.plotting.markers import SingleMarker


class MantidAxesTest(unittest.TestCase):
    """
    Just test if mantid projection works
    """

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5, 3, 5],
            DataE=[1, 2, 3, 4, 1, 1],
            NSpec=3,
            Distribution=True,
            UnitX="Wavelength",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_histo",
        )

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace("ws2d_histo")

    def setUp(self):
        self.fig, self.ax = plt.subplots(subplot_kw={"projection": "mantid"})

    def tearDown(self):
        plt.close("all")
        self.fig, self.ax = None, None

    def test_line2d_plots(self):
        self.ax.plot(self.ws2d_histo, "rs", specNum=2, linewidth=6)
        self.assertEqual("r", self.ax.lines[-1].get_color())
        self.ax.plot(np.arange(10), np.arange(10), "bo-")

    def test_errorbar_plots(self):
        self.ax.errorbar(self.ws2d_histo, specNum=2, linewidth=6)
        self.ax.errorbar(np.arange(10), np.arange(10), 0.1 * np.ones((10,)), fmt="bo-")

    def test_imshow(self):
        self.ax.imshow(self.ws2d_histo)

    def test_pcolor(self):
        self.ax.pcolor(self.ws2d_histo)

    def test_pcolorfast(self):
        self.ax.pcolorfast(self.ws2d_histo)

    def test_pcolormesh(self):
        self.ax.pcolormesh(self.ws2d_histo)

    def test_remove_workspace_artist_for_known_workspace_removes_plot(self):
        self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)
        workspace_removed = self.ax.remove_workspace_artists(self.ws2d_histo)
        self.assertEqual(True, workspace_removed)
        self.assertEqual(0, len(self.ax.lines))

    def test_remove_workspace_artist_for_unknown_workspace_does_nothing(self):
        self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)
        unknown_ws = CreateSampleWorkspace()
        workspace_removed = self.ax.remove_workspace_artists(unknown_ws)
        self.assertEqual(False, workspace_removed)
        self.assertEqual(1, len(self.ax.lines))

    def test_remove_workspace_artist_for_removes_only_specified_workspace(self):
        second_ws = CreateSampleWorkspace()
        line_ws2d_histo = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_second_ws = self.ax.plot(second_ws, specNum=5)[0]
        self.assertEqual(2, len(self.ax.lines))

        workspace_removed = self.ax.remove_workspace_artists(self.ws2d_histo)
        self.assertEqual(True, workspace_removed)
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo not in self.ax.lines)
        self.assertTrue(line_second_ws in self.ax.lines)
        DeleteWorkspace(second_ws)

    def test_remove_workspace_artist_with_predicate_removes_only_lines_from_specified_workspace_which_return_true(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        self.ax.remove_artists_if(lambda artist: artist.get_label() == "ws2d_histo: 6")
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)

    def test_workspace_artist_object_correctly_removed_if_all_lines_removed(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        is_empty = self.ax.remove_artists_if(lambda artist: artist.get_label() in ["ws2d_histo: 6", "ws2d_histo: 8"])
        self.assertEqual(0, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 not in self.ax.lines)
        self.assertEqual(self.ax.tracked_workspaces, {})
        self.assertTrue(is_empty)

    def test_remove_if_correctly_prunes_workspace_artist_list(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        self.ax.remove_artists_if(lambda artist: artist.get_label() == "ws2d_histo: 6")
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)
        self.assertEqual(self.ax.tracked_workspaces[self.ws2d_histo.name()][0]._artists, [line_ws2d_histo_spec_3])

    def test_remove_if_removes_untracked_artists(self):
        line = self.ax.plot([0], [0])[0]
        err_cont = self.ax.errorbar([0], [0])
        img = self.ax.imshow([[0, 1], [0, 1]])

        self.ax.remove_artists_if(lambda art: art in [line, err_cont, img])
        self.assertNotIn(line, self.ax.lines)
        self.assertNotIn(err_cont[0], self.ax.lines)
        self.assertNotIn(err_cont, self.ax.containers)
        self.assertNotIn(img, self.ax.images)

    def test_remove_if_correctly_removes_lines_associated_with_multiple_workspaces(self):
        second_ws = CreateSampleWorkspace()
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        line_second_ws = self.ax.plot(second_ws, specNum=5)[0]
        self.assertEqual(3, len(self.ax.lines))

        is_empty = self.ax.remove_artists_if(lambda artist: artist.get_label() in ["ws2d_histo: 6", "second_ws: spec 5"])
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)
        self.assertTrue(line_second_ws not in self.ax.lines)
        self.assertEqual(len(self.ax.tracked_workspaces), 1)
        self.assertFalse(is_empty)
        DeleteWorkspace(second_ws)

    def test_replace_workspace_data_plot(self):
        plot_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30], DataY=[3, 4, 5, 3, 4, 5], DataE=[1, 2, 3, 4, 1, 1], NSpec=3)
        line_ws2d_histo = self.ax.plot(plot_data, specNum=2, color="r")[0]
        plot_data = CreateWorkspace(DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40], DataY=[3, 4, 5, 3, 4, 5], DataE=[1, 2, 3, 4, 1, 1], NSpec=3)
        self.ax.replace_workspace_artists(plot_data)
        self.assertAlmostEqual(25, line_ws2d_histo.get_xdata()[0])
        self.assertAlmostEqual(35, line_ws2d_histo.get_xdata()[-1])
        self.assertEqual("r", line_ws2d_histo.get_color())
        # try deleting
        self.ax.remove_workspace_artists(plot_data)

    def test_replace_workspace_data_plot_with_fewer_spectra(self):
        plot_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30], DataY=[3, 4, 5, 3, 4, 5], DataE=[1, 2, 3, 4, 1, 1], NSpec=3)
        line_ws2d_histo_spec_2 = self.ax.plot(plot_data, specNum=2, color="r")[0]

        plot_data = CreateWorkspace(DataX=[20, 30, 40, 20, 30, 40], DataY=[3, 4, 3, 4], DataE=[1, 2, 1, 2], NSpec=2)
        self.ax.replace_workspace_artists(plot_data)
        self.assertAlmostEqual(25, line_ws2d_histo_spec_2.get_xdata()[0])
        self.assertAlmostEqual(35, line_ws2d_histo_spec_2.get_xdata()[-1])
        self.assertEqual("r", line_ws2d_histo_spec_2.get_color())
        # try deleting
        self.ax.remove_workspace_artists(plot_data)

    def test_replace_workspace_data_errorbar(self):
        eb_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30], DataY=[3, 4, 5, 3, 4, 5], DataE=[1, 2, 3, 4, 1, 1], NSpec=3)
        self.ax.errorbar(eb_data, specNum=2, color="r")
        eb_data = CreateWorkspace(
            DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40], DataY=[3, 4, 5, 3, 4, 5], DataE=[0.1, 0.2, 0.3, 0.4, 0.1, 0.1], NSpec=3
        )
        self.ax.replace_workspace_artists(eb_data)
        self.assertEqual(1, len(self.ax.containers))
        eb_container = self.ax.containers[0]
        self.assertTrue(isinstance(eb_container, ErrorbarContainer))
        self.assertAlmostEqual(25, eb_container[0].get_xdata()[0])
        self.assertAlmostEqual(35, eb_container[0].get_xdata()[-1])
        self.assertEqual("r", eb_container[0].get_color())
        # try deleting
        self.ax.remove_workspace_artists(eb_data)

    def _do_image_replace_common_bins(self, color_func, artists_func, kwargs=None, modifier_func=None, extra_checks=None):
        """Do checks for replacing a workspace that backs an image-type-plot

        :param color_func: The Axes callable to use to create the plot
        :param artists_func: A callable that will return the appropriate artists from the axes
        :param kwargs: dict Optional kwargs to pass to plot creation
        :param modifier_func: An optional function to apply to the axes after creation.
        :param extra_checks: An optional function to run after the initial assertions have been made
        """
        im_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30], DataY=[3, 4, 5, 3, 4, 5], DataE=[1, 2, 3, 4, 1, 1], NSpec=3)
        if kwargs is None:
            kwargs = dict()
        # Do plot and perform any requested modifications
        color_func(im_data, **kwargs)
        if modifier_func is not None:
            modifier_func(self.ax)

        # Replace the workspace
        im_data = CreateWorkspace(
            DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40],
            DataY=[3, 4, 5, 3, 4, 5],
            DataE=[0.1, 0.2, 0.3, 0.4, 0.1, 0.1],
            NSpec=3,
            VerticalAxisValues=[2, 3, 4],
            VerticalAxisUnit="DeltaE",
        )

        self.ax.replace_workspace_artists(im_data)

        artists = artists_func(self.ax)
        self.assertEqual(1, len(artists))
        left, right, bottom, top = get_colorplot_extents(artists[0])
        self.assertAlmostEqual(20.0, left)
        self.assertAlmostEqual(40.0, right)
        self.assertAlmostEqual(1.5, bottom)
        self.assertAlmostEqual(4.5, top)
        if extra_checks is not None:
            extra_checks(self.ax)

        # try deleting
        self.ax.remove_workspace_artists(im_data)

    def test_replace_workspace_data_imshow_default_norm(self):
        self._do_image_replace_common_bins(self.ax.imshow, lambda ax: ax.images)

    def test_replace_workspace_data_imshow_with_norm_on_creation(self):
        self._do_image_replace_common_bins(self.ax.imshow, lambda ax: ax.images, kwargs={"norm": Normalize()})

    def test_replace_workspace_data_imshow_keeps_cmap_changed_after_creation(self):
        new_cmap = "copper"

        def switch_cmap(axes):
            axes.images[0].set_cmap(new_cmap)

        def extra_checks(axes):
            self.assertEqual(new_cmap, axes.images[0].get_cmap().name)

        self._do_image_replace_common_bins(self.ax.imshow, lambda ax: ax.images, modifier_func=switch_cmap, extra_checks=extra_checks)

    def test_replace_workspace_data_imshow_keeps_properties(self):
        interpolation = "none"

        def use_no_interpolation(axes):
            axes.images[0].set_interpolation(interpolation)

        def extra_checks(axes):
            self.assertEqual(interpolation, axes.images[0].get_interpolation())

        self._do_image_replace_common_bins(
            self.ax.imshow, lambda ax: ax.images, modifier_func=use_no_interpolation, extra_checks=extra_checks
        )

    def test_replace_workspace_data_pcolor(self):
        self._do_image_replace_common_bins(self.ax.pcolor, lambda ax: ax.collections)

    def test_replace_workspace_data_pcolorfast(self):
        self._do_image_replace_common_bins(self.ax.pcolorfast, lambda ax: ax.collections)

    def test_replace_workspace_data_pcolormesh(self):
        self._do_image_replace_common_bins(self.ax.pcolormesh, lambda ax: ax.collections)

    def test_3d_plots(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        ax.plot(self.ws2d_histo, specNum=1)
        ax.plot(np.arange(10), np.arange(10), np.arange(10))
        ax.plot_wireframe(self.ws2d_histo)

    def test_fail(self):
        fig, ax = plt.subplots()
        self.assertRaises(Exception, ax.plot, self.ws2d_histo, "rs", specNum=1)
        self.assertRaises(Exception, ax.pcolormesh, self.ws2d_histo)

    def test_fail_3d(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="3d")
        self.assertRaises(Exception, ax.plot_wireframe, self.ws2d_histo)
        self.assertRaises(Exception, ax.plot_surface, self.ws2d_histo)

    def test_plot_is_not_normalized_for_bin_plots(self):
        workspace = CreateWorkspace(
            DataX=[10, 20], DataY=[2, 3, 4, 5, 6], DataE=[1, 2, 1, 2, 1], NSpec=5, Distribution=False, OutputWorkspace="workspace"
        )
        self.ax.plot(workspace, specNum=1, axis=MantidAxType.BIN, distribution=False)
        self.ax.plot(workspace, specNum=1, axis=MantidAxType.BIN, distribution=True)
        self.ax.plot(workspace, specNum=1, axis=MantidAxType.BIN)
        ws_artists = self.ax.tracked_workspaces[workspace.name()]
        self.assertFalse(ws_artists[0].is_normalized)
        self.assertFalse(ws_artists[1].is_normalized)
        self.assertFalse(ws_artists[2].is_normalized)

    def test_artists_normalization_state_labeled_correctly_for_dist_workspace(self):
        dist_ws = CreateWorkspace(DataX=[10, 20], DataY=[2, 3], DataE=[1, 2], NSpec=1, Distribution=True, OutputWorkspace="dist_workpace")
        self.ax.plot(dist_ws, specNum=1, distribution=False)
        self.ax.plot(dist_ws, specNum=1, distribution=True)
        self.ax.plot(dist_ws, specNum=1)
        ws_artists = self.ax.tracked_workspaces[dist_ws.name()]
        self.assertTrue(ws_artists[0].is_normalized)
        self.assertTrue(ws_artists[1].is_normalized)
        self.assertTrue(ws_artists[2].is_normalized)

    def test_artists_normalization_state_labeled_correctly_for_non_dist_workspace(self):
        non_dist_ws = CreateWorkspace(
            DataX=[10, 20], DataY=[2, 3], DataE=[1, 2], NSpec=1, Distribution=False, OutputWorkspace="non_dist_workpace"
        )
        self.ax.plot(non_dist_ws, specNum=1, distribution=False)
        self.assertTrue(self.ax.tracked_workspaces[non_dist_ws.name()][0].is_normalized)
        del self.ax.tracked_workspaces[non_dist_ws.name()]

        self.ax.errorbar(non_dist_ws, specNum=1, distribution=True)
        self.assertFalse(self.ax.tracked_workspaces[non_dist_ws.name()][0].is_normalized)
        del self.ax.tracked_workspaces[non_dist_ws.name()]

        auto_dist = config["graph1d.autodistribution"] == "On"
        self.ax.plot(non_dist_ws, specNum=1)
        self.assertEqual(auto_dist, self.ax.tracked_workspaces[non_dist_ws.name()][0].is_normalized)
        del self.ax.tracked_workspaces[non_dist_ws.name()]

    def test_artists_normalization_state_labeled_correctly_for_non_dist_workspace_and_global_setting_off(self):
        non_dist_ws = CreateWorkspace(
            DataX=[10, 20, 25, 30], DataY=[2, 3, 4, 5], DataE=[1, 2, 1, 2], NSpec=1, Distribution=False, OutputWorkspace="non_dist_workpace"
        )
        config["graph1d.autodistribution"] = "Off"
        self.ax.plot(non_dist_ws, specNum=1)
        self.assertFalse(self.ax.tracked_workspaces[non_dist_ws.name()][0].is_normalized)
        del self.ax.tracked_workspaces[non_dist_ws.name()]

    def test_artists_normalization_state_labeled_correctly_for_2d_plots_of_dist_workspace(self):
        plot_funcs = ["imshow", "pcolor", "pcolormesh", "pcolorfast", "tripcolor", "contour", "contourf", "tricontour", "tricontourf"]
        dist_2d_ws = CreateWorkspace(
            DataX=[10, 20, 10, 20], DataY=[2, 3, 2, 3], DataE=[1, 2, 1, 2], NSpec=2, Distribution=True, OutputWorkspace="non_dist_workpace"
        )
        for plot_func in plot_funcs:
            func = getattr(self.ax, plot_func)
            func(dist_2d_ws, distribution=False)
            func(dist_2d_ws, distribution=True)
            func(dist_2d_ws)
            ws_artists = self.ax.tracked_workspaces[dist_2d_ws.name()]
            self.assertTrue(ws_artists[0].is_normalized)
            self.assertTrue(ws_artists[1].is_normalized)
            self.assertTrue(ws_artists[2].is_normalized)

    def test_artists_normalization_labeled_correctly_for_2d_plots_of_non_dist_workspace_and_dist_argument_false(self):
        plot_funcs = ["imshow", "pcolor", "pcolormesh", "pcolorfast", "tripcolor", "contour", "contourf", "tricontour", "tricontourf"]
        non_dist_2d_ws = CreateWorkspace(
            DataX=[10, 20, 10, 20], DataY=[2, 3, 2, 3], DataE=[1, 2, 1, 2], NSpec=2, Distribution=False, OutputWorkspace="non_dist_workpace"
        )
        for plot_func in plot_funcs:
            func = getattr(self.ax, plot_func)
            func(non_dist_2d_ws, distribution=False)
            self.assertTrue(self.ax.tracked_workspaces[non_dist_2d_ws.name()][0].is_normalized)
            del self.ax.tracked_workspaces[non_dist_2d_ws.name()]

    def test_artists_normalization_labeled_correctly_for_2d_plots_of_non_dist_workspace_and_dist_argument_true(self):
        plot_funcs = ["imshow", "pcolor", "pcolormesh", "pcolorfast", "tripcolor", "contour", "contourf", "tricontour", "tricontourf"]
        non_dist_2d_ws = CreateWorkspace(
            DataX=[10, 20, 10, 20], DataY=[2, 3, 2, 3], DataE=[1, 2, 1, 2], NSpec=2, Distribution=False, OutputWorkspace="non_dist_workpace"
        )
        for plot_func in plot_funcs:
            func = getattr(self.ax, plot_func)
            func(non_dist_2d_ws, distribution=True)
            self.assertFalse(self.ax.tracked_workspaces[non_dist_2d_ws.name()][0].is_normalized)
            del self.ax.tracked_workspaces[non_dist_2d_ws.name()]

    def test_artists_normalization_labeled_correctly_for_2d_plots_of_non_dist_workspace_and_global_setting_on(self):
        plot_funcs = ["imshow", "pcolor", "pcolormesh", "pcolorfast", "tripcolor", "contour", "contourf", "tricontour", "tricontourf"]
        non_dist_2d_ws = CreateWorkspace(
            DataX=[10, 20, 10, 20], DataY=[2, 3, 2, 3], DataE=[1, 2, 1, 2], NSpec=2, Distribution=False, OutputWorkspace="non_dist_workpace"
        )
        for plot_func in plot_funcs:
            auto_dist = config["graph1d.autodistribution"] == "On"
            func = getattr(self.ax, plot_func)
            func(non_dist_2d_ws)
            self.assertEqual(auto_dist, self.ax.tracked_workspaces[non_dist_2d_ws.name()][0].is_normalized)
            del self.ax.tracked_workspaces[non_dist_2d_ws.name()]

    def test_artists_normalization_labeled_correctly_for_2d_plots_of_non_dist_workspace_and_global_setting_off(self):
        plot_funcs = ["imshow", "pcolor", "pcolormesh", "pcolorfast", "tripcolor", "contour", "contourf", "tricontour", "tricontourf"]
        non_dist_2d_ws = CreateWorkspace(
            DataX=[10, 20, 25, 30, 10, 20, 25, 30],
            DataY=[2, 3, 4, 5, 2, 3, 4, 5],
            DataE=[1, 2, 1, 2, 1, 2, 1, 2],
            NSpec=2,
            Distribution=False,
            OutputWorkspace="non_dist_workpace",
        )
        for plot_func in plot_funcs:
            config["graph1d.autodistribution"] = "Off"
            func = getattr(self.ax, plot_func)
            func(non_dist_2d_ws)
            self.assertFalse(self.ax.tracked_workspaces[non_dist_2d_ws.name()][0].is_normalized)
            del self.ax.tracked_workspaces[non_dist_2d_ws.name()]

    def test_check_axes_distribution_consistency_mixed_normalization(self):
        mock_logger = self._run_check_axes_distribution_consistency([True, False, True])
        mock_logger.assert_called_once_with("You are overlaying distribution and non-distribution data!")

    def test_check_axes_distribution_consistency_all_normalized(self):
        mock_logger = self._run_check_axes_distribution_consistency([True, True, True])
        self.assertEqual(0, mock_logger.call_count)

    def test_check_axes_distribution_consistency_all_non_normalized(self):
        mock_logger = self._run_check_axes_distribution_consistency([False, False, False])
        self.assertEqual(0, mock_logger.call_count)

    def test_that_autoscaling_can_be_turned_off_when_data_changes(self):
        """We should be able to plot a workspace without having it
        autoscale if the workspace changes
        """
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20], OutputWorkspace="ws")
        self.ax.plot(ws, autoscale_on_update=False)
        CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], OutputWorkspace="ws")
        self.assertLess(self.ax.get_ylim()[1], 5000)

    def test_that_autoscaling_can_be_turned_off_when_plotting_multiple_workspaces(self):
        """We should be able to plot a new workspace without having the plot scale to it"""
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20])
        self.ax.plot(ws)
        ws2 = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000])
        self.ax.plot(ws2, autoscale_on_update=False)
        self.assertLess(self.ax.get_ylim()[1], 5000)

    def test_that_plot_autoscales_by_default(self):
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20], OutputWorkspace="ws")
        self.ax.plot(ws)
        ws2 = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], OutputWorkspace="ws2")
        self.ax.plot(ws2)
        self.assertGreaterEqual(self.ax.get_ylim()[1], 5000)

    def test_that_errorbar_autoscales_by_default(self):
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20], DataE=[1, 1], OutputWorkspace="ws")
        self.ax.errorbar(ws)
        ws2 = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], DataE=[1, 1], OutputWorkspace="ws2")
        self.ax.errorbar(ws2)
        self.assertGreaterEqual(self.ax.get_ylim()[1], 5000)

    def test_that_errorbar_autoscaling_can_be_turned_off_when_plotting_multiple_workspaces(self):
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20])
        self.ax.errorbar(ws)
        ws2 = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000])
        self.ax.errorbar(ws2, autoscale_on_update=False)
        self.assertLess(self.ax.get_ylim()[1], 5000)

    def test_that_errorbar_autoscaling_can_be_turned_off(self):
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 20], DataE=[1, 2], OutputWorkspace="ws")
        self.ax.errorbar(ws)
        ws2 = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], DataE=[1, 1], OutputWorkspace="ws2")
        self.ax.errorbar(ws2, autoscale_on_update=False)
        self.assertLess(self.ax.get_ylim()[1], 5000)

    def test_that_plotting_ws_without_giving_spec_num_sets_spec_num_if_ws_has_1_histogram(self):
        ws_name = "ws-with-one-spec"
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], DataE=[1, 1], OutputWorkspace=ws_name)
        self.ax.plot(ws)
        ws_artist = self.ax.tracked_workspaces[ws_name][0]
        self.assertEqual(1, ws_artist.spec_num)
        self.assertTrue("specNum" in self.ax.creation_args[0])
        self.assertFalse("wkspIndex" in self.ax.creation_args[0])

    def test_that_plotting_ws_without_giving_spec_num_raises_if_ws_has_more_than_1_histogram(self):
        self.assertRaises(RuntimeError, self.ax.plot, self.ws2d_histo)

    def test_that_plotting_ws_without_giving_spec_num_sets_correct_spec_num_after_spectra_removed(self):
        CreateWorkspace(DataX=[10, 20, 30], DataY=[10, 20, 30], DataE=[1, 1, 1], NSpec=3, OutputWorkspace="ws-with-3-spec")
        RemoveSpectra("ws-with-3-spec", [0, 1], OutputWorkspace="out_ws")
        out_ws = ADS.retrieve("out_ws")
        self.ax.plot(out_ws)
        ws_artist = self.ax.tracked_workspaces["out_ws"][0]
        self.assertEqual(3, ws_artist.spec_num)

    def test_that_plotting_ws_without_spec_num_adds_default_spec_num_to_creation_args(self):
        ws_name = "ws-with-one-spec"
        ws = CreateWorkspace(DataX=[10, 20], DataY=[10, 5000], DataE=[1, 1], OutputWorkspace=ws_name)
        self.ax.plot(ws)
        self.assertEqual(1, self.ax.creation_args[0]["specNum"])

    def test_that_relim_ignores_interactive_markers(self):
        """
        When calling .relim the content limits of the axes is calculated
        and a margin added. When the axes is resized the interactive
        markers re-calculate their limits to be the new axes limits,
        i.e. the original limits plus the margin. This mean repeatedly
        calling relim and autoscale will zoom out on the axes.

        This test makes sure this isn't happening and that the markers
        are ignored when calling .relim in this case.
        """
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        y_min, y_max = ax.get_ylim()
        SingleMarker(fig.canvas, "g", 0.5, y_min, y_max, marker_type="XSingle", axis=ax)
        ax.relim()
        ax.autoscale()
        np.testing.assert_almost_equal((y_min, y_max), ax.get_ylim())

    def test_converting_from_1d_plot_to_waterfall_plot(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        # Plot the same line twice.
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)

        self.assertTrue(ax.is_waterfall())
        # Check the lines' data are different now that it is a waterfall plot.
        self.assertNotEqual(ax.get_lines()[0].get_xdata()[0], ax.get_lines()[1].get_xdata()[0])
        self.assertNotEqual(ax.get_lines()[0].get_ydata()[0], ax.get_lines()[1].get_ydata()[0])

    def test_converting_from_waterfall_plot_to_1d_plot(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        # Plot the same line twice.
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Make the plot non-waterfall again.
        ax.set_waterfall(False)

        self.assertFalse(ax.is_waterfall())
        # Check that the lines have the same x and y data.
        self.assertEqual(ax.get_lines()[0].get_xdata()[0], ax.get_lines()[1].get_xdata()[0])
        self.assertEqual(ax.get_lines()[0].get_ydata()[0], ax.get_lines()[1].get_ydata()[0])

    def test_converting_waterfall_plot_scale(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        # Plot the same line twice.
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        x_lin_1 = ax.get_lines()[1].get_xdata()[0]
        y_lin_1 = ax.get_lines()[1].get_ydata()[0]

        ax.set_xscale("log")
        ax.set_yscale("log")
        x_log = ax.get_lines()[1].get_xdata()[0]
        y_log = ax.get_lines()[1].get_ydata()[0]

        ax.set_xscale("linear")
        ax.set_yscale("linear")
        x_lin_2 = ax.get_lines()[1].get_xdata()[0]
        y_lin_2 = ax.get_lines()[1].get_ydata()[0]

        self.assertTrue(ax.is_waterfall())
        # Check the lines' data are different now that it is a log scale waterfall plot.
        self.assertNotEqual(x_lin_1, x_log)
        self.assertNotEqual(y_lin_1, y_log)
        self.assertEqual(x_lin_1, x_lin_2)
        self.assertEqual(y_lin_1, y_lin_2)

    def test_create_fill_creates_fills_for_waterfall_plot(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Add filled areas.
        ax.set_waterfall_fill(True)

        fills = datafunctions.get_waterfall_fills(ax)
        self.assertEqual(len(fills), 2)

    def test_remove_fill_removes_fills_for_waterfall_plots(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Add filled areas.
        ax.set_waterfall_fill(True)
        # Remove filled areas.
        ax.set_waterfall_fill(False)

        self.assertFalse(ax.waterfall_has_fill())

    def test_converting_from_waterfall_to_1d_plot_removes_filled_areas(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Add filled areas.
        ax.set_waterfall_fill(True)
        # Make the plot non-waterfall again.
        ax.set_waterfall(False)

        self.assertFalse(ax.waterfall_has_fill())

    def test_overplotting_onto_waterfall_plot_with_line_colour_fills_adds_another_filled_area_with_new_line_colour(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1], color="#ff9900")
        ax.plot([0, 1], [0, 1], color="#00d1ff")

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Add filled areas.
        ax.set_waterfall_fill(True)
        # Set the fills to be the same colour as their lines.
        ax.collections[0].set_facecolor(ax.lines[0].get_color())
        ax.collections[1].set_facecolor(ax.lines[0].get_color())

        # Plot another line and make it join the waterfall.
        ax.plot([0, 1], [0, 1], color="#00fff0")
        datafunctions.convert_single_line_to_waterfall(ax, 2)
        datafunctions.waterfall_update_fill(ax)

        # Check that there are now three filled areas and the new line colour matches the new fill colour.
        self.assertEqual(convert_color_to_hex(ax.collections[2].get_facecolor()[0]), ax.lines[2].get_color())

    def test_overplotting_onto_waterfall_plot_with_solid_colour_fills_adds_a_filled_area_with_the_same_colour(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot.
        ax.set_waterfall(True)
        # Add filled areas.
        ax.set_waterfall_fill(True)
        # Set the fills to be the same colour.
        ax.collections[0].set_facecolor([1, 0, 0, 1])
        ax.collections[1].set_facecolor([1, 0, 0, 1])

        # Plot another line and make it join the waterfall.
        ax.plot([0, 1], [0, 1])
        datafunctions.convert_single_line_to_waterfall(ax, 2)
        datafunctions.waterfall_update_fill(ax)

        # Check that there are now three filled areas and the new fill colour matches the others.
        self.assertTrue((ax.collections[2].get_facecolor() == [1, 0, 0, 1]).all())

    def test_fills_not_created_if_waterfall_plot_already_has_filled_areas(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot
        ax.set_waterfall(True)

        # Add filled areas twice
        ax.set_waterfall_fill(True)
        ax.set_waterfall_fill(True)

        # There should still be only two filled areas (one for each line)
        self.assertEqual(len(datafunctions.get_waterfall_fills(ax)), 2)

    def test_increasing_waterfall_x_offset_will_autoscale_x_axis(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot
        ax.set_waterfall(True, x_offset=20)
        x_high = ax.get_xlim()[1]

        # Increase x offset
        ax.update_waterfall(x_offset=40, y_offset=ax.waterfall_y_offset)

        self.assertGreater(ax.get_xlim()[1], x_high)

    def test_decreasing_waterfall_x_offset_will_autoscale_x_axis(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot
        ax.set_waterfall(True, x_offset=40)
        x_high = ax.get_xlim()[1]

        # Decrease x offset
        ax.update_waterfall(x_offset=20, y_offset=ax.waterfall_y_offset)

        self.assertLess(ax.get_xlim()[1], x_high)

    def test_increasing_waterfall_y_offset_will_autoscale_y_axis(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot
        ax.set_waterfall(True, y_offset=20)
        y_high = ax.get_ylim()[1]

        # Increase y off set
        ax.update_waterfall(x_offset=ax.waterfall_x_offset, y_offset=40)

        self.assertGreater(ax.get_ylim()[1], y_high)

    def test_decreasing_waterfall_y_offset_will_autoscale_y_axis(self):
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid"})
        ax.plot([0, 1], [0, 1])
        ax.plot([0, 1], [0, 1])

        # Make a waterfall plot
        ax.set_waterfall(True, y_offset=40)
        y_high = ax.get_ylim()[1]

        # Decrease y off set
        ax.update_waterfall(x_offset=ax.waterfall_x_offset, y_offset=20)

        self.assertLess(ax.get_ylim()[1], y_high)

    def test_imshow_with_origin_upper(self):
        image = self.ax.imshow(self.ws2d_histo, origin="upper")

        # 0,0 in ax coordinates is the bottom left of figure, add 0.5 to move into canvas
        xy_pixels = self.ax.transAxes.transform((0, 0)) + (0.5, 0.5)
        bottom_left_corner = MouseEvent("motion_notify_event", self.fig.canvas, x=xy_pixels[0], y=xy_pixels[1])

        self.assertEqual(image.get_extent(), [10.0, 30.0, 9.0, 3.0])
        self.assertEqual(image.get_cursor_data(bottom_left_corner), 3.0)

    def test_imshow_with_origin_lower(self):
        image = self.ax.imshow(self.ws2d_histo, origin="lower")

        # 0,0 in ax coordinates is the bottom left of figure, add 0.5 to move into canvas
        xy_pixels = self.ax.transAxes.transform((0, 0)) + (0.5, 0.5)
        bottom_left_corner = MouseEvent("motion_notify_event", self.fig.canvas, x=xy_pixels[0], y=xy_pixels[1])

        self.assertEqual(image.get_extent(), [10.0, 30.0, 3.0, 9.0])
        self.assertEqual(image.get_cursor_data(bottom_left_corner), 2.0)

    def test_rename_workspace_relabels_curve_if_default_label(self):
        ws = CreateSampleWorkspace()
        self.ax.plot(ws, specNum=2)
        expected_name = "ws: spec 2"
        expected_new_name = "new_name: spec 2"
        ws_artist = self.ax.tracked_workspaces["ws"][0]
        artist = ws_artist._artists[0]
        self.assertEqual(artist.get_label(), expected_name)
        self.ax.rename_workspace(new_name="new_name", old_name="ws")
        self.assertEqual(artist.get_label(), expected_new_name)

    def test_rename_workspace_relabels_curve_if_default_label_for_numeric_axis(self):
        self.ax.plot(self.ws2d_histo, specNum=2)
        expected_name = "ws2d_histo: 6"
        expected_new_name = "new_name: 6"
        ws_artist = self.ax.tracked_workspaces["ws2d_histo"][0]
        artist = ws_artist._artists[0]
        self.assertEqual(artist.get_label(), expected_name)
        self.ax.rename_workspace(new_name="new_name", old_name="ws2d_histo")
        self.assertEqual(artist.get_label(), expected_new_name)

    def test_rename_workspace_does_not_relabel_curve_if_name_not_default(self):
        ws = CreateSampleWorkspace()
        self.ax.plot(ws, specNum=2)
        new_label = "Curve 1"
        ws_artist = self.ax.tracked_workspaces["ws"][0]
        artist = ws_artist._artists[0]
        artist.set_label(new_label)
        self.ax.rename_workspace(new_name="new_name", old_name="ws")
        self.assertEqual(artist.get_label(), new_label)

    def test_rename_workspace_updates_creation_args(self):
        ws = CreateSampleWorkspace()
        self.ax.plot(ws, specNum=2)
        self.assertEqual("ws", self.ax.creation_args[0]["workspaces"])
        self.ax.rename_workspace(new_name="new_name", old_name="ws")
        self.assertEqual("new_name", self.ax.creation_args[0]["workspaces"])

    def test_rename_workspace_works_with_additional_line_plot(self):
        ws = CreateSampleWorkspace()
        self.ax.plot(ws, specNum=2)
        self.ax.axvline(x=1, label="label", color="red")
        self.assertIsNone(self.ax.creation_args[1].get("workspaces"))
        # Line plot has no workspaces creation arg, test there is no exception
        self.ax.rename_workspace(new_name="new_name", old_name="ws")

    def _run_check_axes_distribution_consistency(self, normalization_states):
        mock_tracked_workspaces = {
            "ws": [Mock(is_normalized=normalization_states[0]), Mock(is_normalized=normalization_states[1])],
            "ws1": [Mock(is_normalized=normalization_states[2])],
        }
        with patch("mantid.kernel.logger.warning", Mock()) as mock_logger:
            with patch.object(self.ax, "tracked_workspaces", mock_tracked_workspaces):
                self.ax.check_axes_distribution_consistency()
        return mock_logger


if __name__ == "__main__":
    unittest.main()
