# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import

# std imports
from unittest import TestCase, main

# thirdparty imports
import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

# local imports
from mantidqt.plotting.figuretype import figure_type, FigureType


class FigureTypeTest(TestCase):
    def test_figure_type_empty_figure_returns_empty(self):
        self.assertEqual(FigureType.Empty, figure_type(plt.figure()))

    def test_subplot_with_multiple_plots_returns_other(self):
        ax = plt.subplot(221)
        self.assertEqual(FigureType.Other, figure_type(ax.figure))

    def test_line_plot_returns_line(self):
        ax = plt.subplot(111)
        ax.plot([1])
        self.assertEqual(FigureType.Line, figure_type(ax.figure))

    def test_line_plot_returns_line_when_ax_specified(self):
        ax = plt.subplot(111)
        ax.plot(range(3))
        self.assertEqual(FigureType.Line, figure_type(ax.figure, ax))

    def test_error_plot_returns_error(self):
        ax = plt.subplot(111)
        ax.errorbar([1], [1], yerr=[0.01])
        self.assertEqual(FigureType.Errorbar, figure_type(ax.figure))

    def test_image_plot_returns_image(self):
        ax = plt.subplot(111)
        ax.imshow([[1], [1]])
        self.assertEqual(FigureType.Image, figure_type(ax.figure))

    def test_surface_plot_returns_surface(self):
        a = np.array([[1]])
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})
        ax.plot_surface(a, a, a)
        self.assertEqual(FigureType.Surface, figure_type(ax.figure))

    def test_wireframe_plot_returns_wireframe(self):
        a = np.array([[1]])
        ax = plt.subplot(111, projection="3d")
        ax.plot_wireframe(a, a, a)
        self.assertEqual(FigureType.Wireframe, figure_type(ax.figure))

    def test_contour_plot_returns_contour(self):
        ax = plt.subplot(111)
        ax.imshow([[1], [1]])
        ax.contour([[1, 1], [1, 1]])
        self.assertEqual(FigureType.Contour, figure_type(ax.figure))

    def test_mesh_plot_returns_mesh(self):
        a = np.array([[[1, 1, 1], [2, 2, 2], [3, 3, 3]]])
        mesh_polygon = Poly3DCollection(a)
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})
        ax.add_collection3d(mesh_polygon)
        self.assertEqual(FigureType.Mesh, figure_type(ax.figure))


if __name__ == "__main__":
    main()
