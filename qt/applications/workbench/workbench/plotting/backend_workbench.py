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
"""
Qt-based matplotlib backend that can operate when called from non-gui threads.

It uses qtagg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# std imports
import importlib


# 3rd party imports
# Put these first so that the correct Qt version is selected by qtpy
from qtpy import QT_VERSION

# local imports
from workbench.plotting.figuremanager import (backend_version,  # noqa
    draw_if_interactive as draw_if_interactive_impl,
    new_figure_manager as new_figure_manager_impl,
    new_figure_manager_given_figure as new_figure_manager_given_figure_impl,
    show as show_impl,
    QAppThreadCall
)

# Import the *real* matplotlib backend for the canvas
mpl_qtagg_backend = importlib.import_module('matplotlib.backends.backend_qt{}agg'.format(QT_VERSION[0]))
try:
    FigureCanvas = getattr(mpl_qtagg_backend, 'FigureCanvasQTAgg')
except KeyError:
    raise ImportError("Unknown form of matplotlib Qt backend.")


# -----------------------------------------------------------------------------
# Backend implementation
# -----------------------------------------------------------------------------
show = QAppThreadCall(show_impl)
new_figure_manager = QAppThreadCall(new_figure_manager_impl)
new_figure_manager_given_figure = QAppThreadCall(new_figure_manager_given_figure_impl)
draw_if_interactive = QAppThreadCall(draw_if_interactive_impl)
