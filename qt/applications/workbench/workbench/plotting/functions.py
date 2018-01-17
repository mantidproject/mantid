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
"""Defines a collection of functions to support plotting workspaces with
our custom window.
"""

# std imports

# 3rd party imports
from matplotlib.figure import Figure

# local imports
from workbench.plotting.currentfigure import CurrentFigure
from workbench.plotting.figuremanager import LinePlotFigureManagerQT

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = 'mantid'


# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def plot_spectrum(workspace, spectrum_nums=None, indices=None):
    # window/figure creation
    figure_mgr = CurrentFigure.get_fig_manager()
    if figure_mgr is None:
        figure_mgr = new_figure_manager()
    CurrentFigure.set_active(figure_mgr)
    fig = figure_mgr.canvas.figure

    # do plotting
    ax = fig.add_subplot(111, projection=PROJECTION)
    return ax.plot(workspace, specNum=spectrum_nums, wkspIndex=indices)


def new_figure_manager():
    allfigs = CurrentFigure.figs()
    next_num = max(allfigs) + 1 if allfigs else 1
    figure = Figure(next_num)
    return LinePlotFigureManagerQT(figure, next_num)
