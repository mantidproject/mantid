# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Qt-based matplotlib backend that can operate when called from non-gui threads.

It uses qtagg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# std imports
import importlib

# local imports
from matplotlib.backends.backend_qt5agg import (draw_if_interactive as draw_if_interactive_impl,
                                                show as show_impl)  # noqa
# 3rd party imports
# Put these first so that the correct Qt version is selected by qtpy
from qtpy import QT_VERSION

from workbench.plotting.figuremanager import (QAppThreadCall, new_figure_manager as new_figure_manager_impl,
                                              new_figure_manager_given_figure as new_figure_manager_given_figure_impl)

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
