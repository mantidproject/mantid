# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Qt-based matplotlib backend that can operate when called from non-gui threads.

It uses qtagg for rendering but the ensures that any rendering calls
are done on the main thread of the application as the default
"""

from workbench.plotting.figuremanager import (
    FigureCanvasQTAgg, QAppThreadCall, draw_if_interactive_impl, show_impl, new_figure_manager as
    new_figure_manager_impl, new_figure_manager_given_figure as
    new_figure_manager_given_figure_impl)

# -----------------------------------------------------------------------------
# Backend implementation
# -----------------------------------------------------------------------------
FigureCanvas = FigureCanvasQTAgg
draw_if_interactive = QAppThreadCall(draw_if_interactive_impl)
show = QAppThreadCall(show_impl)
# These are wrapped by figuremanager
new_figure_manager = new_figure_manager_impl
new_figure_manager_given_figure = new_figure_manager_given_figure_impl
