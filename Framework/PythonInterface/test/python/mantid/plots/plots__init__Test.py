# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import matplotlib

matplotlib.use('AGG')
import matplotlib.pyplot as plt
from matplotlib.container import ErrorbarContainer
import numpy as np
import unittest

from mantid.api import WorkspaceFactory
from mantid.plots.plotfunctions import get_colorplot_extents
from mantid.py3compat.mock import Mock, patch
from mantid.simpleapi import (AnalysisDataService, CreateWorkspace,
                              CreateSampleWorkspace, DeleteWorkspace)


class Plots__init__Test(unittest.TestCase):
    '''
    Just test if mantid projection works
    '''

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5, 3, 5],
                                         DataE=[1, 2, 3, 4, 1, 1],
                                         NSpec=3,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace('ws2d_histo')

    def setUp(self):
        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'mantid'})

    def tearDown(self):
        plt.close('all')
        self.fig, self.ax = None, None

    def test_line2d_plots(self):
        self.ax.plot(self.ws2d_histo, 'rs', specNum=2, linewidth=6)
        self.assertEqual('r', self.ax.lines[-1].get_color())
        self.ax.plot(np.arange(10), np.arange(10), 'bo-')

    def test_errorbar_plots(self):
        self.ax.errorbar(self.ws2d_histo, specNum=2, linewidth=6)
        self.ax.errorbar(np.arange(10), np.arange(10), 0.1*np.ones((10,)), fmt='bo-')

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
        is_empty = self.ax.remove_workspace_artists(self.ws2d_histo)
        self.assertEqual(True, is_empty)
        self.assertEqual(0, len(self.ax.lines))

    def test_remove_workspace_artist_for_unknown_workspace_does_nothing(self):
        self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)
        unknown_ws = CreateSampleWorkspace()
        self.ax.remove_workspace_artists(unknown_ws)
        self.assertEqual(1, len(self.ax.lines))

    def test_remove_workspace_artist_for_removes_only_specified_workspace(self):
        second_ws = CreateSampleWorkspace()
        line_ws2d_histo = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_second_ws = self.ax.plot(second_ws, specNum=5)[0]
        self.assertEqual(2, len(self.ax.lines))

        self.ax.remove_workspace_artists(self.ws2d_histo)
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo not in self.ax.lines)
        self.assertTrue(line_second_ws in self.ax.lines)
        DeleteWorkspace(second_ws)

    def test_remove_workspace_artist_with_predicate_removes_only_lines_from_specified_workspace_which_return_true(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        self.ax.remove_artists_if(lambda artist: artist.get_label() == 'ws2d_histo: spec 2')
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)

    def test_workspace_artist_object_correctly_removed_if_all_lines_removed(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        is_empty = self.ax.remove_artists_if(lambda artist: artist.get_label() in ['ws2d_histo: spec 2', 'ws2d_histo: spec 3'])
        self.assertEqual(0, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 not in self.ax.lines)
        self.assertEqual(self.ax.tracked_workspaces, {})
        self.assertTrue(is_empty)

    def test_remove_if_correctly_prunes_workspace_artist_list(self):
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        self.assertEqual(2, len(self.ax.lines))

        self.ax.remove_artists_if(lambda artist: artist.get_label() == 'ws2d_histo: spec 2')
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)
        self.assertEqual(self.ax.tracked_workspaces[self.ws2d_histo.name()][0]._artists, [line_ws2d_histo_spec_3])

    def test_remove_if_correctly_removes_lines_associated_with_multiple_workspaces(self):
        second_ws = CreateSampleWorkspace()
        line_ws2d_histo_spec_2 = self.ax.plot(self.ws2d_histo, specNum=2, linewidth=6)[0]
        line_ws2d_histo_spec_3 = self.ax.plot(self.ws2d_histo, specNum=3, linewidth=6)[0]
        line_second_ws = self.ax.plot(second_ws, specNum=5)[0]
        self.assertEqual(3, len(self.ax.lines))

        is_empty = self.ax.remove_artists_if(lambda artist: artist.get_label() in ['ws2d_histo: spec 2', 'second_ws: spec 5'])
        self.assertEqual(1, len(self.ax.lines))
        self.assertTrue(line_ws2d_histo_spec_2 not in self.ax.lines)
        self.assertTrue(line_ws2d_histo_spec_3 in self.ax.lines)
        self.assertTrue(line_second_ws not in self.ax.lines)
        self.assertEqual(len(self.ax.tracked_workspaces), 1)
        self.assertFalse(is_empty)
        DeleteWorkspace(second_ws)

    def test_replace_workspace_data_plot(self):
        plot_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                    DataY=[3, 4, 5, 3, 4, 5],
                                    DataE=[1, 2, 3, 4, 1, 1],
                                    NSpec=3)
        line_ws2d_histo = self.ax.plot(plot_data, specNum=2, color='r')[0]
        plot_data = CreateWorkspace(DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40],
                                    DataY=[3, 4, 5, 3, 4, 5],
                                    DataE=[1, 2, 3, 4, 1, 1],
                                    NSpec=3)
        self.ax.replace_workspace_artists(plot_data)
        self.assertAlmostEqual(25, line_ws2d_histo.get_xdata()[0])
        self.assertAlmostEqual(35, line_ws2d_histo.get_xdata()[-1])
        self.assertEqual('r', line_ws2d_histo.get_color())
        # try deleting
        self.ax.remove_workspace_artists(plot_data)

    def test_replace_workspace_data_errorbar(self):
        eb_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                  DataY=[3, 4, 5, 3, 4, 5],
                                  DataE=[1, 2, 3, 4, 1, 1],
                                  NSpec=3)
        self.ax.errorbar(eb_data, specNum=2, color='r')
        eb_data = CreateWorkspace(DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40],
                                  DataY=[3, 4, 5, 3, 4, 5],
                                  DataE=[.1, .2, .3, .4, .1, .1],
                                  NSpec=3)
        self.ax.replace_workspace_artists(eb_data)
        self.assertEqual(1, len(self.ax.containers))
        eb_container = self.ax.containers[0]
        self.assertTrue(isinstance(eb_container, ErrorbarContainer))
        self.assertAlmostEqual(25, eb_container[0].get_xdata()[0])
        self.assertAlmostEqual(35, eb_container[0].get_xdata()[-1])
        self.assertEqual('r', eb_container[0].get_color())
        # try deleting
        self.ax.remove_workspace_artists(eb_data)

    def _do_image_replace_common_bins(self, color_func, artists):
        im_data = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                  DataY=[3, 4, 5, 3, 4, 5],
                                  DataE=[1, 2, 3, 4, 1, 1],
                                  NSpec=3)
        getattr(self.ax, color_func)(im_data)
        im_data = CreateWorkspace(DataX=[20, 30, 40, 20, 30, 40, 20, 30, 40],
                                  DataY=[3, 4, 5, 3, 4, 5],
                                  DataE=[.1, .2, .3, .4, .1, .1],
                                  NSpec=3, VerticalAxisValues=[2, 3, 4],
                                  VerticalAxisUnit='DeltaE')
        self.ax.replace_workspace_artists(im_data)
        self.assertEqual(1, len(artists))
        left, right, bottom, top = get_colorplot_extents(artists[0])
        self.assertAlmostEqual(20., left)
        self.assertAlmostEqual(40., right)
        self.assertAlmostEqual(1.5, bottom)
        self.assertAlmostEqual(4.5, top)
        # try deleting
        self.ax.remove_workspace_artists(im_data)

    def test_replace_workspace_data_imshow(self):
        self._do_image_replace_common_bins('imshow', self.ax.images)

    def test_replace_workspace_data_pcolor(self):
        self._do_image_replace_common_bins('pcolor', self.ax.collections)

    def test_replace_workspace_data_pcolorfast(self):
        self._do_image_replace_common_bins('pcolorfast', self.ax.collections)

    def test_replace_workspace_data_pcolormesh(self):
        self._do_image_replace_common_bins('pcolormesh', self.ax.collections)

    def test_3d_plots(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='mantid3d')
        ax.plot(self.ws2d_histo, specNum=1)
        ax.plot(np.arange(10), np.arange(10), np.arange(10))
        ax.plot_wireframe(self.ws2d_histo)

    def test_fail(self):
        fig, ax = plt.subplots()
        self.assertRaises(Exception, ax.plot, self.ws2d_histo, 'rs', specNum=1)
        self.assertRaises(Exception, ax.pcolormesh, self.ws2d_histo)

    def test_fail_3d(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        self.assertRaises(Exception, ax.plot_wireframe, self.ws2d_histo)
        self.assertRaises(Exception, ax.plot_surface, self.ws2d_histo)

    def test_artists_normalization_state_labeled_correctly_dist_workspace(self):
        dist_ws = CreateWorkspace(DataX=[10, 20],
                                  DataY=[2, 3],
                                  DataE=[1, 2],
                                  NSpec=1,
                                  Distribution=True,
                                  OutputWorkspace='dist_workpace')
        self.ax.plot(dist_ws, specNum=1, distribution=False)
        self.ax.plot(dist_ws, specNum=1, distribution=True)
        ws_artists = self.ax.tracked_workspaces[dist_ws.name()]
        self.assertTrue(ws_artists[0].is_normalized)
        self.assertTrue(ws_artists[1].is_normalized)

    def test_artists_normalization_state_labeled_correctly_non_dist_workspace(self):
        non_dist_ws = CreateWorkspace(DataX=[10, 20],
                                      DataY=[2, 3],
                                      DataE=[1, 2],
                                      NSpec=1,
                                      Distribution=False,
                                      OutputWorkspace='non_dist_workpace')
        self.ax.plot(non_dist_ws, specNum=1, distribution=False)
        self.ax.plot(non_dist_ws, specNum=1, distribution=True)
        ws_artists = self.ax.tracked_workspaces[non_dist_ws.name()]
        self.assertTrue(ws_artists[0].is_normalized)
        self.assertFalse(ws_artists[1].is_normalized)

    def test_check_axes_distribution_consistency_mixed_normalization(self):
        mock_logger = self._run_check_axes_distribution_consistency(
            [True, False, True])
        mock_logger.assert_called_once_with("You are overlaying distribution and "
                                            "non-distribution data!")

    def test_check_axes_distribution_consistency_all_normalized(self):
        mock_logger = self._run_check_axes_distribution_consistency(
            [True, True, True])
        self.assertEqual(0, mock_logger.call_count)

    def test_check_axes_distribution_consistency_all_non_normalized(self):
        mock_logger = self._run_check_axes_distribution_consistency(
            [False, False, False])
        self.assertEqual(0, mock_logger.call_count)

    def _run_check_axes_distribution_consistency(self, normalization_states):
        mock_tracked_workspaces = {
            'ws': [Mock(is_normalized=normalization_states[0]),
                    Mock(is_normalized=normalization_states[1])],
            'ws1': [Mock(is_normalized=normalization_states[2])]}
        with patch('mantid.kernel.logger.warning', Mock()) as mock_logger:
            with patch.object(self.ax, 'tracked_workspaces', mock_tracked_workspaces):
                self.ax.check_axes_distribution_consistency()
        return mock_logger


if __name__ == '__main__':
    unittest.main()
