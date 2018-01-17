#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
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
"""Provides our custom figure manager to wrap the canvas, window and our custom toolbar"""

# std imports
import importlib

# 3rdparty imports
from qtpy import QT_VERSION
mpl_qtagg_backend = importlib.import_module('matplotlib.backends.backend_qt{}agg'.format(QT_VERSION[0]))

# local imports
from workbench.plotting.toolbars.lineplot_toolbar import LinePlotNavigationToolbar


class LinePlotFigureManagerQT(mpl_qtagg_backend.FigureManagerQT):

    def __init__(self, figure, num):
        canvas = mpl_qtagg_backend.FigureCanvasQT(figure)
        super(LinePlotFigureManagerQT, self).__init__(canvas, num)

    def _get_toolbar(self, canvas, parent):
        return LinePlotNavigationToolbar(canvas, parent, False)
