#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__  import absolute_import

# std imports
from unittest import TestCase, main

# thirdparty imports
import matplotlib
matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt

# local imports
from workbench.plotting.figuretype import figure_type, FigureType


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

    def test_image_plot_returns_image(self):
        ax = plt.subplot(111)
        ax.imshow([[1],[1]])
        self.assertEqual(FigureType.Image, figure_type(ax.figure))


if __name__ == '__main__':
    main()
