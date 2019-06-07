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

from mantid.plots.plotfunctions import get_colorplot_extents
from mantid.api import WorkspaceFactory
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


if __name__ == '__main__':
    unittest.main()
